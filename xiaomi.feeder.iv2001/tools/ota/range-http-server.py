#!/usr/bin/env python3
"""Minimal HTTP server that supports Range requests for OTA delivery."""

import argparse
import os
import sys
from http.server import HTTPServer, SimpleHTTPRequestHandler


class RangeHTTPRequestHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        path = self.translate_path(self.path)
        if not os.path.isfile(path):
            super().do_GET()
            return

        file_size = os.path.getsize(path)
        range_header = self.headers.get("Range")

        if range_header is None:
            super().do_GET()
            return

        try:
            range_spec = range_header.strip().removeprefix("bytes=")
            start_str, end_str = range_spec.split("-", 1)
            start = int(start_str)
            end = int(end_str) if end_str else file_size - 1
        except (ValueError, AttributeError):
            self.send_error(416, "Invalid Range")
            return

        if start >= file_size or end >= file_size or start > end:
            self.send_error(416, "Range Not Satisfiable")
            return

        length = end - start + 1
        self.send_response(206)
        ctype = self.guess_type(path)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(length))
        self.send_header("Content-Range", f"bytes {start}-{end}/{file_size}")
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("Connection", "close")
        self.end_headers()

        with open(path, "rb") as f:
            f.seek(start)
            self.wfile.write(f.read(length))
        self.wfile.flush()

    def log_message(self, format, *args):
        if os.environ.get("RANGE_HTTP_QUIET") == "1":
            return
        sys.stderr.write("%s - %s\n" % (self.address_string(), format % args))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", type=int, nargs="?", default=8080)
    parser.add_argument("--bind", default="0.0.0.0")
    parser.add_argument("--directory", default=".")
    args = parser.parse_args()

    os.chdir(args.directory)
    server = HTTPServer((args.bind, args.port), RangeHTTPRequestHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    server.server_close()


if __name__ == "__main__":
    main()
