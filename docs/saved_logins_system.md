# Saved Logins System — How It Works

A full end-to-end explanation of how previously used User IDs are saved,
persisted, and shown in the login screen panel.

---

## Layer 1 — Physical Storage: Windows Registry

Qt's `QSettings` class writes the saved IDs directly into the **Windows Registry**:

```
HKEY_CURRENT_USER
  └── Software
        └── testQtProject
              └── LoginApp
                    └── recentLogins = ["user123", "user456", "user789"]
```

You can inspect this live by opening `regedit.exe` and navigating to that path.

- Data is stored as a plain string list (no encryption — passwords are **never** stored)
- Writes are atomic, so data survives app crashes immediately
- Persists across reboots and reinstalls (until the registry key is deleted)

---

## Layer 2 — C++ Class: `recentlogins.h`

```cpp
QSettings s("testQtProject", "LoginApp");
//            ↑ matches registry path above
```

The class exposes three `Q_INVOKABLE` methods (callable from QML):

| Method | What it does |
|---|---|
| `all()` | Opens registry → reads list → returns it as `QStringList` |
| `add(id)` | Reads list → removes duplicate → prepends id → caps at 10 → writes back |
| `remove(id)` | Reads list → removes that id → writes back |

Every call opens and closes the registry directly — no in-memory cache.
This guarantees data is never stale even if multiple instances run.

---

## Layer 3 — Bridge to QML: `main.cpp`

```cpp
RecentLogins recentLogins;
engine.rootContext()->setContextProperty("RecentLogins", &recentLogins);
```

This injects the C++ object into QML's global scope under the name `"RecentLogins"`.
Every QML file loaded by this engine can call `RecentLogins.all()` etc. as plain JS.

The `Q_OBJECT` macro + `Q_INVOKABLE` tags on each method are what make this possible.

---

## Layer 4 — QML: `LoginPage.qml`

```qml
property var _recentList: []          // local display copy, not the source of truth

function refreshList() {
    _recentList = RecentLogins.all()  // pulls fresh data from registry via C++
}
```

`_recentList` is a **temporary display copy** for the Repeater to render.
The registry is always the real source of truth.

---

## Full Data Flow

### App startup
```
Component.onCompleted
  → refreshList()
    → RecentLogins.all()          [C++ reads registry]
      → returns ["user123", ...]
        → _recentList updated
          → Repeater renders the saved logins panel
```

### User clicks a saved login entry
```
itemMouse.onClicked
  → idField.text = modelData      [fills the ID field — nothing written]
  → passwordField.forceActiveFocus()
```
No registry write. Just autofills the form.

### Successful login
```
attemptLogin()
  → LicenseAuth.validateCredentials(id, pwd) → true
    → RecentLogins.add(id)        [C++ writes to registry]
    → refreshList()               [C++ reads back → panel updates]
    → loginPage.loginSucceeded(id)
```

### User clicks X on an entry
```
removeMouse.onClicked
  → RecentLogins.remove(modelData)  [C++ writes to registry]
  → refreshList()                   [C++ reads back → panel updates]
```

---

## Architecture Diagram

```
┌─────────────────── Windows Registry ──────────────────────┐
│  HKCU\Software\testQtProject\LoginApp\recentLogins        │
│  Value: ["user123", "user456", "user789"]                 │
└───────────────────────┬───────────────────────────────────┘
                        │  read/write via QSettings
                        │
┌───────────────────────▼───────────────────────────────────┐
│               recentlogins.h  (C++)                       │
│   all()  /  add(id)  /  remove(id)                        │
│   Q_INVOKABLE → visible to QML                            │
└───────────────────────┬───────────────────────────────────┘
                        │  registered as context property
                        │
┌───────────────────────▼───────────────────────────────────┐
│                    main.cpp                               │
│   engine.rootContext()                                    │
│     ->setContextProperty("RecentLogins", &recentLogins)  │
└───────────────────────┬───────────────────────────────────┘
                        │  called as plain JavaScript
                        │
┌───────────────────────▼───────────────────────────────────┐
│                LoginPage.qml                              │
│                                                           │
│  property var _recentList: []   <- display copy only      │
│  refreshList() { _recentList = RecentLogins.all() }       │
│                                                           │
│  On startup   → refreshList()                             │
│  On success   → RecentLogins.add(id) → refreshList()      │
│  On X click   → RecentLogins.remove(id) → refreshList()  │
│  On item click → idField.text = modelData                 │
└───────────────────────────────────────────────────────────┘
```

---

## Quick Reference

| Question | Answer |
|---|---|
| Where is data stored? | Windows Registry (`HKCU\Software\testQtProject\LoginApp`) |
| Is it encrypted? | No — plain text User IDs only |
| Does it survive reboots? | Yes — registry is persistent |
| Max entries | 10 (enforced in `add()`) |
| Is the password saved? | **Never** — only the User ID |
| When is data written? | On successful login, or when user removes an entry |
| When is the QML panel updated? | After every `refreshList()` call |
| File that stores the data | None — goes directly to the Windows Registry |
