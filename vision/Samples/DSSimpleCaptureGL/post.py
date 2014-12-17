import requests
import sys

URL = "https://pew-pew-pew.herokuapp.com"


if len(sys.argv) < 4:
    sys.exit(1)

x = sys.argv[1]
y = sys.argv[2]
z = sys.argv[3]

query = ("/position?"
    + "x=" + x + "&"
    + "y=" + y + "&"
    + "z=" + z)

requests.post(URL + query)
