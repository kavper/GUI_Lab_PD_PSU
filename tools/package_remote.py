"""Package the embedded console; run after editing Board/remote.html."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
page=(root/'Board/remote.html').read_text(encoding='utf-8')
page=page.replace('/* REMOTE_SCRIPT */',(root/'Board/remote.js').read_text(encoding='utf-8')+'\n'+(root/'Board/remote_suite.js').read_text(encoding='utf-8'))
page=page.replace('/* REMOTE_STYLE */',(root/'Board/remote_suite.css').read_text(encoding='utf-8'))
data=page.encode('utf-8')
(root/'Board/remote_preview.html').write_bytes(data)
(root/'Board/remote_page.h').write_text('/* Packaged from Board/remote.html by tools/package_remote.py. */\nstatic const char remote_page[] = {\n'+','.join(str(c) for c in data)+',0};\n')
