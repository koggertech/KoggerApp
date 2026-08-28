# Behaviour tests

```bash
node tools/qml_test/test_usbl_plan_logic.mjs
node tools/qml_test/test_usbl_node_logic.mjs
node tools/qml_test/test_usbl_field_logic.mjs
node tools/qml_test/test_stand_logic.mjs
```

765 assertions over four logic modules — no Qt, no window, no GPU, well under a second for all
four. Exit 0 = all pass.

**215 Â· [UsblPlanLogic.js](../../qml/app/UsblPlanLogic.js)** â€” what the plan IS. Every UI
defect the feature shipped is a rule in here, and so is every invariant the model rests on:
the **total partition** (all eight slots owned exactly once, checked as a property over a
sequence of edits rather than in one case), dissolve-on-empty, `def` as a label that follows
the settings rather than a group that exists, duplicate groups being legal and joinable,
`reconstruct` collapsing equal settings and being a fixed point, the derived views agreeing
with each other, `addStep` walking a group's slots instead of repeating the first, schedule
length matching the node cards, the exact bytes of a total `Apply` (all eight slots, every
time), staleness measured in wire bytes so a join never provokes a re-apply, bit-length
consistency, and persistence repairing a blob rather than trusting it â€” including the
v4/v5/v6 â†’ v7 migrations. **Muting a step** is in here too, with the rule that matters most:
toggling one changes not a single byte `Apply` would send, so it can never make the plan read as
stale. A node with every step muted contributes nothing â€” not even the implicit cmd 0 a node with
no steps gets, which would otherwise mean muting the last step silently started interrogating
something else.

**256 · [UsblNodeLogic.js](../../qml/app/UsblNodeLogic.js)** — what a node row and its command
chips REPORT: the interrogation cycle as a reducer. One request is in flight at a time, so an
answer window closes when its reply lands, when the next request goes out, or when its budget
(dwell + grace) runs out â€” and an unanswered window is a *result*, recorded immediately rather
than waiting for a fix to age out. None of that is observable on the wire: the driver's ping
calls are fire-and-forget and the only inbound is a solution, so every verdict is the host's
inference and every way of being wrong about it either accuses a working beacon or exonerates a
dead one.

Three shipped defects are rules in here.

- **The unit is a step, `(node, cmd)`, not a node** â€” `scenario: one command out of four goes
  unanswered` runs a full cycle in which only cmd 2 stays silent and asserts the chips name it
  exactly. Keying this by node made a stale row unattributable: no way to tell which command had
  failed.
- **The single-Step scenario** exists because the loop is stopped in that case and the operation
  axis keyed on "is the loop running", so one press of Step never showed Waiting at all. It now
  walks a whole cycle and asserts the frame, both badges, the chip and the age chip at every step.
- **The row reports its node's LAST interrogation, not the worst of them.** Worst-case was
  tried and fails twice: it calls a node stale whose last exchange succeeded, and it never clears,
  because a *muted* command that went stale before it was muted is never asked again and would
  condemn the row for the rest of the session. Both failures are assertions.
- **No verdict takes a clock argument** â€” asserted on arity, and by holding one entry at seven
  different ages â€” because folding age in made `Replied` flip to `Stale` five seconds after the
  last fix with nothing being interrogated, announcing that a beacon had gone quiet when nobody
  had spoken to it.

There is also an assertion that `stepKey` cannot collide by concatenation: node 1 cmd 12 and node
11 cmd 2 would be one key without a separator, and that collision is silent.

Two later sections defend things that are invisible in *both* a behaviour test and a screenshot,
so they assert over SOURCE TEXT instead:

- **The loop outlives the pane.** `UsblEngine` and `UsblPlanStore` must be declared in
  `MainWindow`, not in `DeviceSettingsPage` — a settings sub-page whose loader destroys it on
  navigation, which is how closing the settings panel used to stop interrogation silently. The
  pane must own no `Timer` and no poll state of its own, dwell must persist and `running` must
  not, and losing the device must stop the loop.
- **The on-scene nodes panel.** It must not compose its own rows (a second implementation of the
  pane is free to disagree with it), its height must follow the row count while its width does
  not, its `Repeater` must be modelled on the count rather than the recomposed array, it must
  send nothing, and both its `Component`s must sit *inside* the delegate — declared beside the
  `Repeater` they cannot see its id, and every binding through it is a `ReferenceError` that
  loads a panel painting nothing. That one shipped and was caught by the UI probe.

Both node-state surfaces keep their own translation tables, because `qsTr`'s context is the file;
the suite asserts that each names exactly the codes the reducer can return, which is what stops
two copies becoming two vocabularies.

**50 Â· [UsblFieldLogic.js](../../qml/kqml_types/UsblFieldLogic.js)** â€” what a data widget
shows: picking the right beacon out of `Dataset.usblSolutions` and rendering it. The failure
being defended is a widget labelled "beacon 2" showing beacon 1's range, which looks
perfectly correct on screen.

The node suite also pins **where the two modules agree and where they deliberately do not**. A
row's reply badge reports an interrogation outcome; a widget's `usblState` reports whether a fix
is usable, which is a fact about age. So they share no verdict â€” asserting the agreement that
used to hold would now assert the age-flip bug back into place â€” but they must still agree on
the beacon (`entryFor` vs `pick`, over every legal address) and on the arithmetic (`ageMs`),
and there is an assertion that the row's `AGE_WARN_MS` is deliberately looser than the widgets'
`STALE_MS` rather than accidentally different.

Both USBL suites additionally assert against **UsblGroup.qml's source text**: that its
translation tables name every code the logic can return and nothing more, and that every
transition the reducer defines is actually driven from the UI. A code with no word renders
blank and a reducer nothing calls is correct and unreachable â€” both invisible in a screenshot.

**101 · [StandLogic.js](../../qml/app/StandLogic.js)** — what a calibration scan IS, and what
the panel may say about it. Four groups. **The firmware's own refusals** are met here rather than
on the wire, because the stand reports nothing: a rejected Start is silence, indistinguishable
from a stand that is slow to move. **The point count** is what survives of the scan drawing that
was cut — an indivisible range still shows up as a count that is not what the operator expected.
**The stale marks** are the whole dirty signal: Start is the only write the stand takes, so
between an edit and the next Start the form and the device disagree, and with no banner and no
readback the struck-through marks are the only place the panel says so. One edit must strike
exactly the marks that depend on it — changing the scan order strikes two, because it renames the
axes and reshapes the grid.

The fourth group asserts over **source text**, for rules invisible in both a behaviour test and a
screenshot: that editing a field sends nothing (`_writeCfg` may persist but never transmit),
that Start is gated on validity and capability but never on a run state the app cannot observe,
that Pause and Resume are two controls rather than one that changes face, that no progress
readout has appeared, that every long string in the panel is a tooltip — a line that toggles with
state changes a floating panel's height while it is being read — and that the panel kind is
hidden without a stand and refuses a second panel. As in the USBL suites, the badge row's
translation table must name exactly the mark codes the logic can return and nothing more.

One defect was found by writing them: `validate` normalised its argument before checking it, so
the clamp repaired every rejectable scan and all four branches were unreachable. It now checks
the configuration as given.

## Why the logic is plain JS and not QML

The tests came second; the reason the logic is extractable at all is that the plan model was
edited **in place** with a `rev` counter as the only change signal. Every binding reading
`_g.ini.reply` rendered once and then silently lied. `UsblPlanLogic.js` returns a new state
from every mutator, so `UsblPlanStore.qml` reassigns one property and QML re-evaluates
everything by itself.

Plain JS also means `node` can load it. Two approaches that do **not** work, both verified,
so nobody repeats them:

1. **Standalone `qml.exe` cannot load the app's QML modules.** The generated
   `kqml_types/qmldir` declares `prefer :/qml/kqml_types/`, so `AppPalette` and `Tokens`
   resolve out of the app binary's resources and its C++ plugin. There is no on-disk module
   to import.
2. **Loading a test `.qml` into the app's own engine instead of `main.qml` segfaults** â€”
   including a trivial `Item` with one `console.log`, so it is the mechanism, not the test.
   The app cannot reach `exec()` without its main window. Not diagnosed.

Anything user-visible stays in QML where `qsTr` lives: the logic module returns issue
**codes** and numbers, and `UsblPlanStore.qml` turns them into translated sentences. That
split is what keeps the rules testable.

## Adding a case

Assert behaviour, not implementation. When a defect reaches an operator, add the rule that
would have caught it here before fixing it â€” four of the first 72 assertions failed on the
first run because they encoded what I assumed rather than what the code did (`addGroup`
deliberately claims the first free slot), and that is the point.
