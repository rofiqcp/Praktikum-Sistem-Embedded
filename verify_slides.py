import re

# Read file
with open(r'c:\github\Praktikum-Sistem-Embedded\Modul-09-FreeRTOS-Queue-Semaphore\notebookllm.md', 'r', encoding='utf-8') as f:
    content = f.read()

# Split by slides
slides = re.split(r'^## SLIDE \d+:', content, flags=re.MULTILINE)[1:]

total_chars = 0
valid_slides = 0

print("=" * 60)
print("SLIDE LENGTH VERIFICATION (Target: 250-280 characters)")
print("=" * 60)

for i, slide in enumerate(slides, 1):
    lines = slide.split('\n')
    text_content = ' '.join([line.strip() for line in lines if line.strip() and not line.startswith('---')])
    text_content = re.sub(r'\s+', ' ', text_content).strip()
    char_count = len(text_content)
    
    if char_count >= 250 and char_count <= 280:
        status = '✓ OK'
        valid_slides += 1
    elif char_count < 250:
        status = '✗ TOO SHORT'
    else:
        status = '✗ TOO LONG'
    
    total_chars += char_count
    print(f"SLIDE {i:2d}: {char_count:3d} chars  {status}")

print("=" * 60)
print(f"Total Slides: {len(slides)}")
print(f"Valid (250-280): {valid_slides}/{len(slides)}")
print(f"Total Characters: {total_chars:,}")
print(f"Average per slide: {total_chars // len(slides):.0f} chars")
print(f"Target total for 45 slides @265 chars avg: {45 * 265:,} chars")
print("=" * 60)
