import os
import PathManager


ALC_POLICY_TEMPLATE_PATH = os.path.join(PathManager.get_resources_path(), "alc_policy/template/ALC_Template.xml")

def populate_alc_policy(revid: str, username: str, userpass: str):
    with open(ALC_POLICY_TEMPLATE_PATH) as f:
        policy = f.read()
        policy = policy.replace("{{revid}}", revid)
        policy = policy.replace("{{username}}", username)
        policy = policy.replace("{{userpass}}", userpass)
        return policy