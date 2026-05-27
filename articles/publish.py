#!/usr/bin/env python3
"""Publish all *.md articles in this directory to yasu-home.com via WP-CLI over SSH."""
import os
import re
import subprocess
import sys
import glob

HERE = os.path.dirname(os.path.abspath(__file__))
HOST = 'xs832503.xsrv.jp'
WP_ROOT = '~/yasu-home.com/public_html'
PHP = '/opt/php-7.4.33/bin/php'
WP = '/usr/bin/wp'
CATEGORY_ID = 40        # IoT・組込み
AUTHOR_ID = 1            # ryuji
REMOTE_TMP = '/tmp/dragon-articles'


def sh(*args, check=True, capture=False):
    """Run a local command."""
    print('+', ' '.join(args), file=sys.stderr)
    if capture:
        return subprocess.run(args, check=check, capture_output=True, text=True).stdout
    return subprocess.run(args, check=check)


def ssh(*remote_argv, capture=False):
    """Run a single command on the remote via ssh, with each arg shell-quoted."""
    # Use a single concatenated remote command; arguments are quoted with shlex
    import shlex
    cmd = ' '.join(shlex.quote(a) for a in remote_argv)
    return sh('ssh', HOST, cmd, capture=capture)


def main():
    sh('ssh', HOST, f'mkdir -p {REMOTE_TMP}')

    md_files = sorted(glob.glob(os.path.join(HERE, '[0-9][0-9]-*.md')))
    print(f'Found {len(md_files)} markdown files', file=sys.stderr)

    posted = []
    for md_path in md_files:
        base = os.path.basename(md_path).removesuffix('.md')
        print(f'\n=== {base} ===', file=sys.stderr)

        # Convert
        out = sh('python3', os.path.join(HERE, 'convert.py'), md_path, capture=True)
        m = re.match(r'TITLE: (.+)\n---BODY---\n', out)
        if not m:
            print(f'  ERROR: cannot parse converter output', file=sys.stderr)
            continue
        title = m.group(1).strip()
        body = out[m.end():]

        body_local = f'/tmp/{base}.body.html'
        with open(body_local, 'w') as f:
            f.write(body)

        body_remote = f'{REMOTE_TMP}/{base}.body.html'
        sh('scp', '-q', body_local, f'{HOST}:{body_remote}')

        print(f'  title: {title}', file=sys.stderr)
        print(f'  body: {os.path.getsize(body_local)} bytes', file=sys.stderr)

        # Run wp post create on the remote, capture porcelain (post ID)
        ssh_cmd = (
            f'cd {WP_ROOT} && '
            f'{PHP} {WP} post create {body_remote} '
            f'--post_type=post '
            f'--post_status=publish '
            f'--post_author={AUTHOR_ID} '
            f'--post_category={CATEGORY_ID} '
            f'--porcelain'
        )
        # Pass title via a separate env var to dodge shell quoting issues
        full = f'export DR_TITLE={shquote(title)} && {ssh_cmd} --post_title="$DR_TITLE"'
        post_id = subprocess.run(
            ['ssh', HOST, full],
            check=True, capture_output=True, text=True
        ).stdout.strip()
        # Filter out any stderr noise; porcelain should print just the ID
        post_id = re.sub(r'.*\n', '', post_id).strip()
        if not post_id.isdigit():
            print(f'  ERROR: expected post ID, got: {post_id!r}', file=sys.stderr)
            continue
        print(f'  posted as ID={post_id}', file=sys.stderr)
        posted.append((post_id, title))

    print('\n=== Published ===')
    for pid, t in posted:
        print(f'  {pid}\t{t}')


def shquote(s: str) -> str:
    """Robust single-quote bash quoting."""
    return "'" + s.replace("'", "'\\''") + "'"


if __name__ == '__main__':
    main()
