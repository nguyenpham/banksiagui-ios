#!/bin/bash
RESOURCE="/${AWS_S3_BUCKET}/${AWS_S3_TARGET_FILE}"
DATE_VALUE=`date -R`
STRING_TO_SIGN="PUT\n\n${S3_SOURCE_CONTENT_TYPE}\n${DATE_VALUE}\n${RESOURCE}"
SIGNATURE=`echo -en ${STRING_TO_SIGN} | openssl sha1 -hmac ${AWS_SECRET_ACCESS_KEY} -binary | base64`
curl -X PUT -T "${S3_SOURCE_FILE}" \
  -H "Host: ${AWS_S3_BUCKET}.s3.amazonaws.com" \
  -H "Date: ${DATE_VALUE}" \
  -H "Content-Type: ${S3_SOURCE_CONTENT_TYPE}" \
  -H "Authorization: AWS ${AWS_ACCESS_KEY_ID}:${SIGNATURE}" \
  https://s3.amazonaws.com/${AWS_S3_BUCKET}/${AWS_S3_TARGET_FILE}