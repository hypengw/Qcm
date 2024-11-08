import base64
import re
import argparse

def decode_string(encoded_str):
    key = b'Encrypt'
    bytes_data = base64.b64decode(encoded_str)
    decoded_bytes = bytearray(bytes_data)

    i = 0
    for j in range(len(decoded_bytes)):
        decoded_bytes[j] ^= key[i]
        i = (i + 1) % len(key)

    return decoded_bytes.decode('utf-8')

def decode_file_strings(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        content = file.read()

    # Replace each base64-encoded string in quotes with its decoded version
    def replace_encoded(match):
        encoded_str = match.group(1)
        try:
            decoded_str = decode_string(encoded_str)
            return f'"{decoded_str}"'
        except Exception as e:
            print(f"Error decoding {encoded_str}: {e}")
            return match.group(0)  # Return original if there's an error

    decoded_content = re.sub(r'"([^"]*?)"', replace_encoded, content)

    # Optionally, write the decoded content back to the file
    with open(file_path, 'w', encoding='utf-8') as file:
        file.write(decoded_content)

def main():
    parser = argparse.ArgumentParser(description="Decode base64-encoded strings in a file.")
    parser.add_argument("file_path", help="Path to the file with base64-encoded strings to decode.")
    args = parser.parse_args()
    
    decode_file_strings(args.file_path)

if __name__ == "__main__":
    main()
