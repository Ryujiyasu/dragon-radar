#!/usr/bin/env python3
"""Convert a markdown article into WordPress Gutenberg block HTML.

Supported markdown features:
- # Title (becomes post title, returned separately)
- ## / ### headings
- paragraphs
- ``` fenced code blocks (with optional language)
- lists (- ...)
- tables (| col | col |)
- inline `code`, **bold**, [text](url)
"""
import sys
import re
import html as H


def esc(s: str) -> str:
    return H.escape(s, quote=False)


def inline(s: str) -> str:
    """Apply inline markdown (code, bold, links). Escape HTML first."""
    out = esc(s)
    out = re.sub(r'`([^`]+)`', lambda m: f'<code>{m.group(1)}</code>', out)
    out = re.sub(r'\*\*([^*]+)\*\*', r'<strong>\1</strong>', out)
    out = re.sub(r'\[([^\]]+)\]\(([^)]+)\)',
                 r'<a href="\2">\1</a>', out)
    return out


def convert(md: str):
    lines = md.split('\n')
    title = None
    blocks = []

    i = 0
    while i < len(lines):
        line = lines[i]

        # Title (# Heading) - only first one captured
        if title is None and line.startswith('# '):
            title = line[2:].strip()
            i += 1
            continue

        # H2
        if line.startswith('## '):
            text = line[3:].strip()
            blocks.append(f'<!-- wp:heading -->\n<h2 class="wp-block-heading">{inline(text)}</h2>\n<!-- /wp:heading -->')
            i += 1
            continue

        # H3
        if line.startswith('### '):
            text = line[4:].strip()
            blocks.append(f'<!-- wp:heading {{"level":3}} -->\n<h3 class="wp-block-heading">{inline(text)}</h3>\n<!-- /wp:heading -->')
            i += 1
            continue

        # Code fence
        if line.startswith('```'):
            lang = line[3:].strip()
            i += 1
            code_lines = []
            while i < len(lines) and not lines[i].startswith('```'):
                code_lines.append(lines[i])
                i += 1
            i += 1  # skip closing ```
            code = '\n'.join(code_lines)
            # Use wp-block-code, escape HTML, preserve newlines
            blocks.append(f'<!-- wp:code -->\n<pre class="wp-block-code"><code>{esc(code)}</code></pre>\n<!-- /wp:code -->')
            continue

        # Table (starts with |...|)
        if line.startswith('|') and '|' in line[1:]:
            # gather table rows
            rows = []
            while i < len(lines) and lines[i].startswith('|'):
                rows.append(lines[i])
                i += 1
            # rows[0] = header, rows[1] = separator (---), rows[2:] = body
            def cells(row):
                return [c.strip() for c in row.strip('|').split('|')]
            if len(rows) >= 2:
                header = cells(rows[0])
                body = [cells(r) for r in rows[2:]]
                t = ['<!-- wp:table -->', '<figure class="wp-block-table"><table><thead><tr>']
                for h in header:
                    t.append(f'<th>{inline(h)}</th>')
                t.append('</tr></thead><tbody>')
                for r in body:
                    t.append('<tr>')
                    for c in r:
                        t.append(f'<td>{inline(c)}</td>')
                    t.append('</tr>')
                t.append('</tbody></table></figure>')
                t.append('<!-- /wp:table -->')
                blocks.append('\n'.join(t))
                continue

        # List (- ...)
        if re.match(r'^[-*] ', line):
            items = []
            while i < len(lines) and re.match(r'^[-*] ', lines[i]):
                items.append(lines[i][2:].rstrip())
                i += 1
            t = ['<!-- wp:list -->', '<ul class="wp-block-list">']
            for it in items:
                t.append(f'<li>{inline(it)}</li>')
            t.append('</ul>')
            t.append('<!-- /wp:list -->')
            blocks.append('\n'.join(t))
            continue

        # Numbered list
        if re.match(r'^\d+\. ', line):
            items = []
            while i < len(lines) and re.match(r'^\d+\. ', lines[i]):
                items.append(re.sub(r'^\d+\. ', '', lines[i]).rstrip())
                i += 1
            t = ['<!-- wp:list {"ordered":true} -->', '<ol class="wp-block-list">']
            for it in items:
                t.append(f'<li>{inline(it)}</li>')
            t.append('</ol>')
            t.append('<!-- /wp:list -->')
            blocks.append('\n'.join(t))
            continue

        # Blank line -> skip
        if line.strip() == '':
            i += 1
            continue

        # Default: paragraph (consume until blank or special line)
        para_lines = []
        while i < len(lines) and lines[i].strip() != '' \
                and not lines[i].startswith('#') \
                and not lines[i].startswith('```') \
                and not lines[i].startswith('|') \
                and not re.match(r'^[-*] ', lines[i]) \
                and not re.match(r'^\d+\. ', lines[i]):
            para_lines.append(lines[i])
            i += 1
        para = ' '.join(l.strip() for l in para_lines)
        if para:
            blocks.append(f'<!-- wp:paragraph -->\n<p>{inline(para)}</p>\n<!-- /wp:paragraph -->')

    return title, '\n\n'.join(blocks)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('usage: convert.py <article.md>', file=sys.stderr)
        sys.exit(2)
    with open(sys.argv[1]) as f:
        md = f.read()
    title, body = convert(md)
    # Print title on first line, then a separator, then body
    print('TITLE:', title)
    print('---BODY---')
    print(body)
