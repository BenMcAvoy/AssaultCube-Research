# Defensive analysis: client-side game manipulation

## Scope

This prototype reads player and entity state from the running AssaultCube client, renders an overlay through a graphics hook, and includes aim-adjustment experiments. It demonstrates how a modified client can expose or act on information available in its own process. It does not implement or evaluate an anti-cheat system.

## Threat model

Assume a player controls their client process and can inspect its local state, change its presentation, or alter the input it sends. A client-only check cannot be the final authority for competitive outcomes.

## Defensive considerations

- Validate movement, firing cadence, hit results, and other competitive outcomes using server-controlled state. Treat client-reported aim and hit information as untrusted input.
- Avoid sending clients information they do not need. If hidden entity data is already present in client memory, hiding it in the interface is not a security boundary.
- Use client-integrity signals as supporting evidence, not as the only control. Combine them with server-side validation and reviewable telemetry, and account for false positives.
- Keep detection claims tied to measured results. This repository does not test how any specific protection detects the prototype.

## Limits

These are design observations, not implemented protections or results from a controlled anti-cheat evaluation. The project is version-specific and its README documents that it is not intended for online or competitive matches.
