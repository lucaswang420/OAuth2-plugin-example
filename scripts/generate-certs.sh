#!/bin/bash
# Generate self-signed TLS certificates for development/testing
# For production, use Let's Encrypt or a proper CA

set -e

CERT_DIR="$(dirname "$0")/../deploy/nginx/ssl"
mkdir -p "$CERT_DIR"

echo "Generating self-signed TLS certificate..."

openssl req -x509 -nodes -days 365 \
  -newkey rsa:2048 \
  -keyout "$CERT_DIR/privkey.pem" \
  -out "$CERT_DIR/fullchain.pem" \
  -subj "/C=US/ST=Dev/L=Local/O=OAuth2Dev/CN=localhost" \
  -addext "subjectAltName=DNS:localhost,DNS:*.localhost,IP:127.0.0.1"

echo "Certificates generated:"
echo "  $CERT_DIR/fullchain.pem"
echo "  $CERT_DIR/privkey.pem"
echo ""
echo "WARNING: These are self-signed certificates for development only."
echo "For production, use Let's Encrypt: certbot certonly --standalone -d your-domain.com"
