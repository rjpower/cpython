import hashlib,subprocess

def state():
    return (subprocess.check_output(['git','status','--porcelain']),
            hashlib.sha256(subprocess.check_output(['git','diff','--binary','HEAD'])).hexdigest())
before=state()
subprocess.run(['make','-C','build/default','-s','regen-cases','clinic'],check=True)
after=state()
assert before==after,(before,after)
print('PASS: regen-cases and clinic leave tracked diff and status unchanged')
print('diff_sha256',after[1])
