---
name: GitHub push access
description: A mismatch between Replit's Git pane connection and the agent-side GitHub integration can block pushes.
---

Replit's Git pane showing GitHub as active does not guarantee the agent's GitHub connection is bound to the current environment. The agent-side integration may still report `not_added`, while `git push` fails authentication.

**Why:** The Git pane and the agent integration registry can reflect separate connection state; repeating the same push or asking for the same token does not repair an unbound connection.

**How to apply:** Before pushing from the agent, verify the GitHub connection is `added` in the current environment. If it remains `not_added`, reconnect or bind it through the current Repl's Integrations flow rather than repeatedly retrying Git CLI authentication.
