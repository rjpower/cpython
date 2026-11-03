# Stack-hit reader and test lifetime

Status: source-only successor to f2a5b3ca33d; no build or native validation.

The reader now latches its prefix length before allocating the result list,
then refreshes the borrowed journal pointer after that allocation. Later
row refreshes, getter clearing and error cleanup remain in place. Journal
growth must not invalidate the first row read.

The existing test discards earlier history while its current Probe is
already held by the C callback's caller, before the original explicit
collection. Numeric addresses can recur across allocation lifetimes. All
original hit-count, type, frame, dropped-record and off-default assertions
remain; stress is neither disabled nor excluded.

The f2 stress11 failure a10a4cc1 remains failed. The one prefix diagnostic
952c57d9 found no earlier matching hits and is inconclusive about that
failure's cause. This change makes no pointer-safety, Probe-death or
collector-correctness claim from that diagnostic. Existing f2/e9 native
products and successful receipts are preserved at their actual identities.
