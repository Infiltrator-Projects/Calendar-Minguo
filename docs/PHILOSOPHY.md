# Engineering Philosophy

Calendar-Minguo adopts the [Infiltrator Engineering Philosophy](https://github.com/Infiltrator-Projects/Libraries/blob/main/docs/PHILOSOPHY.md) as its governing standard. The following requirements are acceptance criteria for this independent calendar provider, not a claim of completed qualification.

## Ownership and fidelity

This provider owns the Minguo calendar's conversion model. Common owns the shared temporal ABI and discovery contract; applications own presentation. Do not duplicate algorithms in consumers or substitute another calendar. Document the applicable epoch, chronology, era and leap rules, day boundary, historical or regional variants, and the provenance and limits of tables or approximations. Distinguish calculated dates from historical observations.

## Correctness and qualification

Specify coordinate units, valid ranges, overflow behaviour and the conditions under which conversion is reversible. Test independent reference dates, boundary transitions, invalid inputs and round-trip properties within the documented domain. Refuse unsupported or ambiguous inputs explicitly rather than silently selecting another model. Keep ABI compatibility, dependencies and release identities explicit; qualify the exact distributed artifacts.

## Craft and documentation

Preserve postgraduate-level reasoning in source comments and technical documents: invariants, derivations, ownership, trade-offs and failure semantics. Apply the family's [visual design authority](https://github.com/Infiltrator-Projects/InfiltratorOS/blob/main/docs/PHILOSOPHY.md) to supplied presentation without changing chronological meaning. A passing build alone is not evidence of historical accuracy.
