from datetime import datetime
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


PHOTOS_DIR = Path(__file__).resolve().parent / "photos"
HOST = "0.0.0.0"
PORT = 5000


class PhotoHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path != "/upload":
            self.send_response(404)
            self.end_headers()
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            length = 0

        if length <= 0:
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"missing image")
            return

        PHOTOS_DIR.mkdir(parents=True, exist_ok=True)
        image = self.rfile.read(length)
        filename = datetime.now().strftime("photo_%Y%m%d_%H%M%S.jpg")
        path = PHOTOS_DIR / filename
        path.write_bytes(image)

        print(f"Saved {path}")
        self.send_response(200)
        self.end_headers()
        self.wfile.write(f"saved {filename}".encode("utf-8"))


if __name__ == "__main__":
    server = HTTPServer((HOST, PORT), PhotoHandler)
    print(f"Listening on http://{HOST}:{PORT}/upload")
    print(f"Photos folder: {PHOTOS_DIR}")
    server.serve_forever()
