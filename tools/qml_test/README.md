# Behaviour tests

```bash
node tools/qml_test/test_usbl_plan_logic.mjs
```

72 assertions over [qml/app/UsblPlanLogic.js](../../qml/app/UsblPlanLogic.js) — no Qt, no
window, no GPU, ~0.1 s. Exit 0 = all pass.

Every UI defect the USBL feature shipped is a rule in here: slot exclusivity, the derived
views agreeing with each other, `addStep` walking a group's slots instead of repeating the
first, schedule length matching the node cards, struct selection (1 section → slot config,
2 → cmd config, a rewrite rule forces the wider one), release tracking scoped to one role's
event, bit-length consistency, and persistence repairing a blob rather than trusting it.

## Why the logic is plain JS and not QML

The tests came second; the reason the logic is extractable at all is that the plan model was
edited **in place** with a `rev` counter as the only change signal. Every binding reading
`_g.ini.reply` rendered once and then silently lied. `UsblPlanLogic.js` returns a new state
from every mutator, so `UsblPlanStore.qml` reassigns one property and QML re-evaluates
everything by itself — see [ui-verification.md](../../docs/KoggerApp-Docs/ui-verification.md).

Plain JS also means `node` can load it. Two approaches that do **not** work, both verified,
so nobody repeats them:

1. **Standalone `qml.exe` cannot load the app's QML modules.** The generated
   `kqml_types/qmldir` declares `prefer :/qml/kqml_types/`, so `AppPalette` and `Tokens`
   resolve out of the app binary's resources and its C++ plugin. There is no on-disk module
   to import.
2. **Loading a test `.qml` into the app's own engine instead of `main.qml` segfaults** —
   including a trivial `Item` with one `console.log`, so it is the mechanism, not the test.
   The app cannot reach `exec()` without its main window. Not diagnosed.

Anything user-visible stays in QML where `qsTr` lives: the logic module returns issue
**codes** and numbers, and `UsblPlanStore.qml` turns them into translated sentences. That
split is what keeps the rules testable.

## Adding a case

Assert behaviour, not implementation. When a defect reaches an operator, add the rule that
would have caught it here before fixing it — four of the first 72 assertions failed on the
first run because they encoded what I assumed rather than what the code did (`addGroup`
deliberately claims the first free slot), and that is the point.
