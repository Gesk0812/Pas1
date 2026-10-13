---
name: GitHub push access
description: A mismatch between Replit's Git pane connection and the agent-side GitHub integration can block pushes.
---

Replit's Git pane and the agent-side GitHub integration can be out of sync. Even when the integration reports `not_added`, an existing project secret may still be able to authenticate a Git push through a temporary askpass helper without exposing its value.

**Why:** Replit's Git pane, integration registry, and shell authentication are separate paths; a failed push attempt does not establish that the saved repository credential is unusable.

**How to apply:** Use the existing credential only through a temporary Git prompt helper, never in a remote URL or output. Before updating an existing branch, fetch and merge remote-only commits; do not force-push. When a genuinely fresh Actions run is requested, push the complete project tree to a new branch and dispatch the workflow on that branch.
