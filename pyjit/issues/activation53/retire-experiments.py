"""Retain rejected round53 native evidence, then retire only its task directories."""

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
EVIDENCE = BASE / 'pyjit-link-wave1/activation53'
SELECTED = BASE / 'worktrees/round51-golden'
HEADS = {
    'completion': '4d40da15b6e2bb9299fa56ef5d08d884a927e7d1',
    'final-homes': '4b339980d020ae4f6026474e8942b7174ce57d16',
    'native-construction': '779a1568a41652736dbd4c93b968d3381a9693b5',
}


def git(source, *args):
    return subprocess.check_output(['git', '-C', str(source), *args], text=True)


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    destination = EVIDENCE / 'retired-experiments'
    destination.mkdir(exist_ok=False)
    sources = {}
    for name, head in HEADS.items():
        source = BASE / 'worktrees' / f'activation53-{name}'
        assert git(source, 'rev-parse', 'HEAD').strip() == head
        assert not git(source, 'status', '--porcelain')
        sources[name] = source

    manifest = []
    for name, source in sources.items():
        (destination / f'{name}.patch').write_text(git(
            source, 'format-patch', '--stdout',
            '0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254..HEAD'))
        (destination / f'{name}-commits.txt').write_text(git(
            source, 'log', '--format=fuller', '--stat',
            '0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254..HEAD'))
        for role in ('release', 'debug'):
            build = BASE / f'activation53-{name}-{role}'
            files = {build / 'python', *build.rglob('*.so')}
            files.update(build / path for path in (
                'Makefile', 'pyconfig.h', 'config.status', 'config.log',
                'pybuilddir.txt', 'Modules/Setup', 'Modules/Setup.local',
                'Modules/Setup.stdlib', 'Modules/Setup.bootstrap'))
            files.update(build.glob('build/lib.*/_sysconfig*'))
            files = sorted(path for path in files if path.is_file())
            rows = [{'source': str(path), 'member': str(path.relative_to(build)),
                     'size': path.stat().st_size, 'sha256': digest(path)}
                    for path in files]
            archive = destination / f'{name}-{role}.tar.gz'
            with tarfile.open(archive, 'x:gz') as stream:
                for path, row in zip(files, rows):
                    stream.add(path, arcname=row['member'], recursive=False)
            with tarfile.open(archive, 'r:gz') as stream:
                for row in rows:
                    with stream.extractfile(row['member']) as member:
                        assert hashlib.file_digest(member, 'sha256').hexdigest() == row['sha256']
            manifest.append({'name': name, 'role': role, 'head': HEADS[name],
                             'archive': str(archive), 'sha256': digest(archive),
                             'files': rows})
            print(f'archived and verified {name}/{role}', flush=True)

    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    removed = []
    for name, source in sources.items():
        assert not git(source, 'status', '--porcelain')
        subprocess.run(['git', '-C', str(SELECTED), 'worktree', 'remove', str(source)], check=True)
        removed.append(str(source))
        for role in ('release', 'debug'):
            build = BASE / f'activation53-{name}-{role}'
            shutil.rmtree(build)
            removed.append(str(build))
    (destination / 'retirement.json').write_text(json.dumps({
        'removed': removed, 'preserved_selected_head': git(SELECTED, 'rev-parse', 'HEAD').strip(),
    }, indent=2) + '\n')
    print(json.dumps({'removed': removed}), flush=True)


if __name__ == '__main__':
    main()
