```bash
GEMINI_API_KEY="a;skdfjaslkdfjaskldjhfajksdf" # <<< IMPORTANT: Replace with your actual API Key!
BASE_URL="https://generativelanguage.googleapis.com" # Define the base URL
IMG_PATH_2="project.png" # Path to your image file

# --- 1. Get MIME type and file size ---
# Make sure the 'file' command is installed (e.g., `sudo apt install file` on Linux)
MIME_TYPE=$(file -b --mime-type "${IMG_PATH_2}")
NUM_BYTES=$(wc -c < "${IMG_PATH_2}" | tr -d ' ') # Use tr -d ' ' to remove leading spaces from wc -c output
DISPLAY_NAME="my_project_image" # A descriptive name for your uploaded file

echo "File: ${IMG_PATH_2}"
echo "MIME Type: ${MIME_TYPE}"
echo "Size: ${NUM_BYTES} bytes"

# --- 2. Initiate Resumable Upload ---
tmp_header_file="upload-header.tmp"

# Initial resumable request to get the upload URL
curl -s "${BASE_URL}/upload/v1beta/files?key=${GEMINI_API_KEY}" \
  -D "${tmp_header_file}" \
  -H "X-Goog-Upload-Protocol: resumable" \
  -H "X-Goog-Upload-Command: start" \
  -H "X-Goog-Upload-Header-Content-Length: ${NUM_BYTES}" \
  -H "X-Goog-Upload-Header-Content-Type: ${MIME_TYPE}" \
  -H "Content-Type: application/json" \
  -d "{'file': {'display_name': '${DISPLAY_NAME}'}}" > /dev/null

# Extract the actual upload URL from the response headers
upload_url=$(grep -i "x-goog-upload-url: " "${tmp_header_file}" | cut -d" " -f2 | tr -d "\r")
rm "${tmp_header_file}" # Clean up temporary header file

# Check if upload_url was successfully extracted
if [ -z "$upload_url" ]; then
  echo "Error: Failed to get upload URL from the API. Check API key and initial request."
  exit 1
fi
echo "Upload URL obtained: ${upload_url}"

# --- 3. Upload the actual bytes ---
curl -s "${upload_url}" \
  -H "Content-Length: ${NUM_BYTES}" \
  -H "X-Goog-Upload-Offset: 0" \
  -H "X-Goog-Upload-Command: upload, finalize" \
  --data-binary "@${IMG_PATH_2}" > file_info.json

# Extract the file_uri
file_uri=$(jq -r ".file.uri" file_info.json) # Use -r for raw string output
echo "file_uri=$file_uri"

# Check if file_uri was successfully extracted
if [ -z "$file_uri" ] || [ "$file_uri" == "null" ]; then
  echo "Error: Failed to get file_uri after upload."
  echo "Raw upload response:"
  cat file_info.json
  exit 1
fi

# --- 4. Now generate content using that file with gemini-2.5-flash ---
MODEL_NAME="gemini-2.5-flash" # <<< Changed to 2.5 flash
MODEL_URL="${BASE_URL}/v1beta/models/${MODEL_NAME}:generateContent?key=${GEMINI_API_KEY}"

# Construct the JSON payload with CORRECT CASING (fileData, mimeType, fileUri)
# IMPORTANT: No comments are allowed inside this JSON block.
read -r -d '' JSON_PAYLOAD << EOF
{
  "contents": [{
    "parts":[
      {"text": "Can you tell me about the instruments in this photo?"},
      {"fileData": {
            "mimeType": "${MIME_TYPE}",
            "fileUri": "${file_uri}"
        }
      }
    ]
  }]
}
EOF

echo "--- Sending request to ${MODEL_NAME} ---"
# Execute the curl command for content generation
curl -s -X POST "${MODEL_URL}" \
    -H 'Content-Type: application/json' \
    -d "$JSON_PAYLOAD" > response.json

# --- 5. Display the response ---
echo "--- Model Response ---"
cat response.json
echo ""

# Attempt to parse and display the text part
echo "--- Extracted Text ---"
jq -r ".candidates[].content.parts[].text" response.json
echo ""
```
