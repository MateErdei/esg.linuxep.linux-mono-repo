
SCRIPT_DIR="${0%/*}"

exec rsync -e "ssh -i ${SCRIPT_DIR}/ssh-keys/regressiontesting.pem" "$@"
