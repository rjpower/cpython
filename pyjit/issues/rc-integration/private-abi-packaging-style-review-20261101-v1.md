# Independent formatting-only packaging review

2026-11-01. **ACCEPT the exact style stage** in capsule `94f20bcb1ca50d52a37a8551dc9577c327439811b57cfeade277524bbdc991ac`, manifest `15c1a1cbede7eb596f40d3ba5cd5fb181d088c22db2a23b68751fe5a79eee705`. This carries the prior prototype and public-layout semantic reviews; it adds no packaging policy or native qualification.

Only the two Python files change. I independently parsed both old/new files and compared their complete ASTs after splitting each multi-name Import into its original ordered single-name imports. The ASTs agree exactly; no import reordering is hidden by normalization. All 74 tool string values and 197 test string values agree, including generated vendor source and archive hashes. Both wheels and the policy note are byte-identical to the accepted public stage. Exact five-file membership, modes and every capsule reference hash passed.

I ran the actual formatted public entry with explicit original wheels and a new reviewer-owned work directory. All 33 tests passed: 28 policy rows and five archive controls. These include byte-identical repacking of both accepted output wheels, original vendor behavior and the actual installer path. The author's Ruff import/format/idempotence logs are bound; this review's independent semantic proof does not depend on trusting the formatter. Extracted reviewer fixtures were removed after replay.

The formatted tool SHA is `e4b2068964ff7f8b35912751a837f263b748e83c24ffe1693e05b5eadf964dc3`; test SHA is `17f5a3c651ae10b7057aa349efa5fab9bf10a6e8fb7c1e65d95f772e12cefe4b`. Selected RC still has HEAD dce654018d5 and exactly the previously recorded five-file public modification set, with the earlier accepted bytes. I changed neither selected source nor staged author files. No target, compiler, installation or native job ran in this review.

Root must apply the formatted text only after its actual idle proof, then freeze the real resulting source revision. A later source/four-profile compiled-input/product bridge must bind that actual revision and dynamic wheel/tool hashes; this formatting review supplies no guessed HEAD or native reuse credit. All original failed native evidence and pending installer/extension/regeneration boundaries remain unchanged.

Evidence: `build/rc-private-abi-packaging-style-review-20261101-v1/`.
