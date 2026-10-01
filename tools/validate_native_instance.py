"""Audit only a private native instance via loopback RCON (never global -hijack)."""
import argparse
import json
from pathlib import Path
import re
import secrets
import socket
import struct
import subprocess
import sys
import time


def packet(request, kind, body):
    data = struct.pack('<ii', request, kind) + body.encode() + b'\0\0'
    return struct.pack('<i', len(data)) + data


class Rcon:
    def __init__(self, port, password):
        self.sock = socket.create_connection(('127.0.0.1', port), timeout=1)
        self.sock.settimeout(2)
        self.sock.sendall(packet(1, 3, password))
        for _ in range(4):
            request, kind, body = self.read()
            if request == -1:
                raise RuntimeError('Private RCON authentication rejected')
            if request == 1 and kind == 2:
                return
        raise RuntimeError('No RCON authentication response')

    def exact(self, count):
        data = b''
        while len(data) < count:
            chunk = self.sock.recv(count - len(data))
            if not chunk:
                raise EOFError('RCON closed')
            data += chunk
        return data

    def read(self):
        length = struct.unpack('<i', self.exact(4))[0]
        if not 10 <= length <= 1024 * 1024:
            raise ValueError('Bad RCON packet size')
        raw = self.exact(length)
        request, kind = struct.unpack_from('<ii', raw)
        return request, kind, raw[8:-2].decode('utf-8', errors='replace')

    def command(self, body):
        self.sock.sendall(packet(2, 2, body))
        result = []
        try:
            while True:
                request, kind, text = self.read()
                if request == 2:
                    result.append(text)
        except (socket.timeout, EOFError, ConnectionResetError):
            return ''.join(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--logs', type=Path, required=True)
    parser.add_argument('--map', default='de_mirage_csgo_new')
    parser.add_argument('--dedicated', action='store_true')
    parser.add_argument('--timeout', type=int, default=120)
    args = parser.parse_args()
    args.logs.mkdir(parents=True, exist_ok=True)
    port = 27666 if args.dedicated else 27668
    password = secrets.token_hex(16)
    name = 'sourceadvanced_private_' + secrets.token_hex(4) + '.cfg'
    cfg = args.runtime / 'cstrike/cfg' / name
    cfg.write_text('sv_lan 1\nsv_cheats 1\nmp_autoteambalance 0\nmp_limitteams 0\n'
                   'sv_hibernate_when_empty 0\nbot_stop 1\nbot_quota_mode normal\nbot_quota 4\nbot_join_after_player 0\nbot_join_team any\nmp_freezetime 0\n'
                   'fps_max 60\nexec sourceadvanced_gameplay.cfg\n', encoding='ascii')
    exe = args.runtime / ('dedicated_launcher.exe' if args.dedicated else 'hl2_launcher.exe')
    options = ['-console'] if args.dedicated else ['-multirun', '-windowed', '-w', '1024', '-h', '768', '-novid']
    cmd = [str(exe), '-game', 'cstrike', *options, '-condebug', '-usercon', '-ip', '127.0.0.1',
           '-port', str(port), '-clientport', '27669', '-tickrate', '128',
           '+sv_lan', '1', '+rcon_password', password, '+servercfgfile', name,
           '+lservercfgfile', name, '+map', args.map]
    log = args.runtime / 'cstrike/console.log'
    before = log.stat().st_size if log.exists() else 0
    proc = subprocess.Popen(cmd, cwd=args.runtime,
                            creationflags=subprocess.CREATE_NO_WINDOW if args.dedicated else 0)
    print('Private native instance PID', proc.pid, 'port', port, flush=True)
    sys.path.insert(0,str(Path(__file__).parent/'arsenal/native_knife_pipeline'))
    from debug_native_crash import monitor
    debug = monitor(proc.pid,args.logs)
    start = time.monotonic()
    responses = {}
    forced = False
    ready = False
    ready_seconds = None
    channel = None
    try:
        while proc.poll() is None and time.monotonic() - start < args.timeout:
            try:
                channel = Rcon(port, password)
                responses['status'] = channel.command('status')
                if ('map' in responses['status'].lower() and args.map in responses['status']):
                    ready = True
                    ready_seconds = round(time.monotonic() - start, 2)
                    break
            except (OSError, EOFError):
                if channel:
                    channel.sock.close()
                    channel = None
            time.sleep(1)
        if ready:
            channel.command('sv_cheats 1; sv_hibernate_when_empty 0; bot_quota_mode normal; bot_join_after_player 0; bot_quota 4; mp_freezetime 0')
            time.sleep(7)
            for command in ('cs_validate_weapon_balance', 'cs_validate_economy', 'cs_economy_status', 'cs_validate_hitboxes', 'cs_audit_player_scale',
                            'cs_export_gunplay', 'sv_simple_ac_status', 'staticprop_validate_handles', 'map_list', 'status'):
                print('Audit',command,flush=True)
                channel.sock.close()
                deadline=time.monotonic()+60
                while True:
                    try:
                        channel = Rcon(port,password)
                        break
                    except (OSError,EOFError):
                        if proc.poll() is not None or time.monotonic()>deadline: raise
                        time.sleep(1)
                responses[command] = channel.command(command)
                (args.logs/'rcon_in_progress.json').write_text(json.dumps(responses,indent=2),encoding='utf8')
                print('Response bytes',len(responses[command]),'process exit',proc.poll(),flush=True)
            channel.command('quit')
            try:
                proc.wait(timeout=20)
            except subprocess.TimeoutExpired:
                forced = True
        else:
            forced = True
    finally:
        if channel:
            channel.sock.close()
        if proc.poll() is None:
            forced = True
            proc.terminate()
            proc.wait(timeout=10)
        cfg.unlink(missing_ok=True)
        debug.join(timeout=5)
    text = (log.read_bytes()[before:] if log.exists() else b'').decode('utf-8', errors='replace')
    all_text = text + '\n' + '\n'.join(responses.values())
    balance = re.findall(r'\[balance-audit\] checked=(\d+) failed=(\d+)', all_text)
    hitboxes = re.findall(r'\[hitbox-audit\] players=(\d+) rays=(\d+) failures=(\d+)', all_text)
    capsules = re.findall(r'mdl_capsules=(\d+)/(\d+)', all_text)
    props = re.findall(r'\[staticprop-audit\] checked=(\d+) high_indices=(\d+) failures=(\d+)', all_text)
    catalog = args.runtime / 'cstrike/cfg/sourceadvanced_maps.txt'
    expected_maps = {line.strip() for line in catalog.read_text().splitlines()
                     if line.strip() and not line.startswith('#')}
    listed_maps = set(re.findall(r'^\s*\[\d+\]\s+(\S+)\s*$', responses.get('map_list', ''), re.M))
    result = dict(dedicated=args.dedicated, map=args.map, pid=proc.pid, ready=ready,
                  ready_seconds=ready_seconds,
                  elapsed=round(time.monotonic() - start, 2), exit=proc.returncode,
                  forced_shutdown=forced, balance=balance[-1] if balance else None,
                  hitboxes=hitboxes[-1] if hitboxes else None, capsules=capsules,
                  restricted_maps=listed_maps == expected_maps,
                  listed_maps=sorted(listed_maps),
                  keyvalues_error='KeyValues Error' in all_text, staticprop_audit=props[-1] if props else None,
                  economy=re.findall(r'\[economy-audit\] checked=(\d+) failures=(\d+)',all_text),
                  server_sha256=__import__('hashlib').sha256((args.runtime/'cstrike/bin/server.dll').read_bytes()).hexdigest(),
                  catalog_sha256=__import__('hashlib').sha256((args.runtime/'cstrike/custom/000_sourceadvanced_catalog.vpk').read_bytes()).hexdigest())
    result['passed'] = bool(ready and proc.returncode == 0 and not forced and balance and
                            balance[-1] == ('34', '0') and hitboxes and int(hitboxes[-1][0]) > 0 and
                            hitboxes[-1][2] == '0' and capsules and
                            all(a == b and int(a) > 0 for a, b in capsules) and result['restricted_maps'] and
                            props and props[-1][2] == '0' and result['economy'] and result['economy'][-1]==('38','0'))
    (args.logs / 'console.log').write_text(text, encoding='utf-8')
    (args.logs / 'rcon.json').write_text(json.dumps(responses, indent=2), encoding='utf-8')
    (args.logs / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2), flush=True)
    raise SystemExit(0 if result['passed'] else 1)


if __name__ == '__main__':
    main()
