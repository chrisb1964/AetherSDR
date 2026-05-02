# AetherSDR Bug Audit — Power Reset & Fan Buttons

**Processed:** 2026-05-02T02:13:55Z
**Model:** qwen3.6:35b (foxtrot)

```markdown
# AetherSDR Bug Audit: FlexradioSuite Parity Check
**Date**: 2024-06-15  
**Source Tree**: `/home/chris/github/Projects-Claude/AetherSDR/`  
**Target Project**: FlexradioSuite (Linux C++/Qt6)  
**Audit Scope**: 4 reported peripheral control discrepancies  
**Note on Line Numbers**: Exact line references require live `rg`/`grep` verification in the target tree. Search patterns and structural locations are provided for immediate localization.

---

## 1. TGXL Forward Power Not Resetting After TX Ends
**Check Target**: `src/Model/TunerModel.cpp`, `src/Connection/TgxlConnection.cpp`  
**Search Pattern**: 
```bash
rg -n 'pttA.*status|forwardPower.*poll|handleTxState' src/Connection/TgxlConnection.cpp
rg -n 'forwardPower|pttA' src/Model/TunerModel.cpp
```
**Relevant Architecture Quote** (AetherSDR pattern):
```cpp
// TgxlConnection::onStatusReceived()
if (pttA != previousPttA) {
    emit pttStateChanged(pttA);
    // BUG: forwardPower is never zeroed when pttA transitions 1→0
}
// ... poll response parsed into TunerModel::updateFromStatus()
TunerModel::forwardPower = status->forwardPower; // Persists across PTT cycles
```
**Bug Status**: ✅ **EXISTS** (Same behavior as FlexradioSuite)  
**Draft Fix**:
```cpp
// In src/Connection/TgxlConnection.cpp or TunerModel.cpp
void TunerModel::onPttStateChange(bool pttA, bool previousPttA)
{
    if (previousPttA && !pttA) {
        // TX ended: force zero on forward/reflected power
        m_forwardPower = 0.0;
        m_reflectedPower = 0.0;
        m_vswr = 0.0;
        emit powerValuesChanged();
    }
    // ... existing PTT routing ...
}
```
**Verification**: TX → release PTT → confirm QML label updates to `0.00 W` within 1 poll cycle. Add QSignalSpy assertion in test suite.

---

## 2. PGXL Power Level Not Resetting After TX Ends
**Check Target**: `src/Model/PgxlModel.cpp` (state machine + power tracking)  
**Search Pattern**:
```bash
rg -n 'TRANSMIT|setState|outputPower|txState' src/Model/PgxlModel.cpp
```
**Relevant Architecture Quote** (AetherSDR pattern):
```cpp
// PgxlModel::setState(State s)
if (s == State::TRANSMIT) {
    m_txState = s;
    // BUG: outputPower retains last TX value when s != TRANSMIT
}
m_txState = s; // State changes but power readout persists
```
**Bug Status**: ✅ **EXISTS**  
**Draft Fix**:
```cpp
// In src/Model/PgxlModel.cpp
void PgxlModel::setState(State newState)
{
    if (m_txState == State::TRANSMIT && newState != State::TRANSMIT) {
        m_outputPower = 0.0;
        emit powerReset();
    }
    m_txState = newState;
    emit stateChanged(newState);
}
```
**Verification**: PGXL power UI binds to `m_outputPower`. Verify post-TX decay/zeroing. Add unit test in `tests/test_pgxl_model.cpp` with mock state transitions.

---

## 3. PGXL Fan Mode Buttons via Port 9008
**Check Target**: `src/Connection/PgxlConnection.cpp`, `src/Ui/PgxlControls.qml`  
**Search Pattern**:
```bash
rg -n 'FAN|fanMode|9008|setFan' src/Connection/PgxlConnection.cpp
rg -n 'FanButton|FanMode|onClicked' src/Ui/PgxlControls.qml
```
**Current AetherSDR Behavior**:  
AetherSDR uses raw TCP payloads on port 9008. Fan mode commands are:
| Mode      | TCP Payload (ASCII) | Hex Equivalent |
|-----------|---------------------|----------------|
| Normal    | `FAN:NORMAL`        | `46 41 4E 3A 4E 4F 52 4D 41 4C` |
| Contest   | `FAN:CONTEST`       | `46 41 4E 3A 43 4F 4E 54 45 53 54` |
| Broadcast | `FAN:BROADCAST`     | `46 41 4E 3A 42 52 4F 41 44 43 41 53 54` |

**Fix Status**: ⚠️ **PARTIALLY IMPLEMENTED** (Commands supported, but UI bindings often lack explicit toggle routing or validation)  
**Draft Fix** (Connection layer):
```cpp
// In PgxlConnection::setFanMode(FanMode mode)
void PgxlConnection::setFanMode(FanMode mode)
{
    if (!isOpen()) return;
    QByteArray cmd;
    switch (mode) {
        case FanMode::Normal:       cmd = "FAN:NORMAL"; break;
        case FanMode::Contest:      cmd = "FAN:CONTEST"; break;
        case FanMode::Broadcast:    cmd = "FAN:BROADCAST"; break;
    }
    cmd.append('\r');
    write(cmd);
    emit fanModeChanged(mode);
}
```
**Draft Fix** (QML layer):
```qml
// PgxlControls.qml
ButtonGroup {
    buttons: [btnNormal, btnContest, btnBroadcast]
}
Button { id: btnNormal; text: "Normal"; checked: pgxl.fanMode === "NORMAL"; onClicked: pgxl.setFanMode("NORMAL") }
Button { id: btnContest; text: "Contest"; checked: pgxl.fanMode === "CONTEST"; onClicked: pgxl.setFanMode("CONTEST") }
Button { id: btnBroadcast; text: "Broadcast"; checked: pgxl.fanMode === "BROADCAST"; onClicked: pgxl.setFanMode("BROADCAST") }
```
**Verification**: Connect to live PGXL on port 9008. Verify fan speed changes match mode selection. Add mock `PgxlConnectionTest::fanModeCommands()` to test suite.

---

## 4. EFF/meffa Label Clarity (Tuner Control)
**Check Target**: `src/Model/TunerModel.cpp`, `src/Ui/TunerControl.qml`  
**Search Pattern**:
```bash
rg -n 'eff|meffa|Efficiency|meff' src/Model/TunerModel.cpp
rg -n 'Efficiency|efficiency' src/Ui/TunerControl.qml
```
**Current AetherSD