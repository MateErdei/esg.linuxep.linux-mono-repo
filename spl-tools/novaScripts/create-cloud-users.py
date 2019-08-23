import requests
from requests.packages.urllib3.exceptions import InsecureRequestWarning
import json
import time

requests.packages.urllib3.disable_warnings(InsecureRequestWarning)


def create_user(email):
    print "Attempting to create User {}".format(email)
    payload = {"email":email,"config":{"country":"us","msp_licensed":False,"srv_adv":[{"suspended":False,"license":{"id":"L0000000000","type":"FULL","entities":99,"starts":"2017-01-11","expiry":"3000-01-01"},"ep_update":{"user":"CSP7I0S0GZZE","pass":"xn28ddszs1q","url":"http://dci.sophosupd.com/cloudupdate"}}],"srv_std":[{"suspended":False,"license":{"id":"L0000000001","type":"FULL","entities":99,"starts":"2017-01-11","expiry":"3000-01-01"},"ep_update":{"user":"CSP7I0S0GZZE","pass":"xn28ddszs1q","url":"http://dci.sophosupd.com/cloudupdate"}}]},"cloud_managed":True}
    with requests.session() as sess:
        resp = sess.post("https://fe.sandbox.sophos/integration/3/customers", json=payload, verify=False)
        try:
            url = json.loads(resp.text)['uri']
        except KeyError:
            print "User {} already exists!".format(email)
            return
    print url
    with requests.session() as sess_two:
        payload = {"accepted_eula":"true","password":"Ch1pm0nk","region":"amzn-eu-west-1"}
        resp = sess_two.get(url, verify=False, allow_redirects=True)

        for i in str(resp.text).split("\n"):
            if "CSRF_TOKEN" in i:
                token = i.split("'")[1]

        sess_two.headers.update({'Referer':'https://fe.sandbox.sophos/manage/setup', 'X-CSRF-Token' : token, 'Content-Type' : 'application/json;charset=UTF-8'})
        resp2 = sess_two.post("https://fe.sandbox.sophos/api/setup/config",json=payload,verify=False)
    if resp2.status_code == 201:
        print "User {} successfully created".format(email)
    return resp2.status_code

if __name__ == "__main__":
    addresses = ["dev@ssplinux.test.com", "dev@savlinux.test.com", "qa@savlinux.xmas.testqa.com", "trunk@savlinux.xmas.testqa.com"]
    for email in addresses:
        tries = 0
        created = False
        while not created and tries < 5:
            tries += 1
            try:
                resp = create_user(email)
                created = True
            except:
                created = False
            time.sleep(2)
