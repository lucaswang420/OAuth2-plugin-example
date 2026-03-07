import os
import glob
import re

directory = 'd:/work/development/Repos/backend/drogon-plugin/OAuth2-plugin-example/OAuth2Backend/test'
files = glob.glob(os.path.join(directory, '*.cc'))

for file_path in files:
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Just insert the timeout check right before the line containing f.get()
    # Match indentation
    new_content = re.sub(r'([ \t]+)(.*f\.get\(\);)', r'\1if(f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) { throw std::runtime_error("TIMEOUT"); }\n\1\2', content)
    
    if new_content != content:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f'Updated {file_path}')
