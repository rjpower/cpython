import gc
import multiprocessing

multiprocessing.Lock()
# Reference counting unlinks the lock's semaphore here, as it dies.
gc.collect()  # For other GCs.


def f():
    print("ok")


if __name__ == "__main__":
    ctx = multiprocessing.get_context("forkserver")
    modname = "test.mp_preload"
    # Make sure it's importable
    __import__(modname)
    ctx.set_forkserver_preload([modname])
    proc = ctx.Process(target=f)
    proc.start()
    proc.join()
