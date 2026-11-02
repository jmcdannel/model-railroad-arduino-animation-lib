# 🔐 Security

These are small Arduino sketches, so the main risks are **leaked
credentials** (a WiFi password or key committed by mistake) and **unsafe
code** (a sketch that could damage hardware, e.g. driving a pin past its
current limit).

## Reporting

Please report privately through
[GitHub's private vulnerability reporting](https://github.com/jmcdannel/model-railroad-arduino-animation-lib/security/advisories/new),
not in a public issue. You'll get a reply within a week.

## If you committed a secret

1. **Rotate it first:** change the WiFi password, revoke the token or
   regenerate the key. Once pushed, assume it's been copied.
2. Then report it privately (above) so the history can be cleaned up.

## How leaks are prevented

- 🛡️ GitHub secret scanning with push protection
- 🤖 A required **Secret scan** CI check (gitleaks with custom Arduino
  rules in `.gitleaks.toml`) over the full history of every PR
- 🪝 A local pre-commit hook: `git config core.hooksPath .githooks`
- 🙈 `arduino_secrets.h`, `*_secrets.h`, `.env` and keys are gitignored

See [CONTRIBUTING.md](CONTRIBUTING.md#-secrets-the-one-hard-rule) for the
rules contributors follow.
