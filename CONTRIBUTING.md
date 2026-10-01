# Contributing to OctaC

Thanks for helping out. The project is still early, so these guidelines are short and will change as the compiler takes shape.

## Workflow

1. Don't commit directly to `main`. Create a branch for each change, named `<type>/<short-description>`, for example `fix/exponent-floats`.
2. Keep each pull request focused on one change.
3. Open a pull request into `main`. At least one other team member should review it before it is merged.

## Types

The same types are used for branch names and commit messages:

- `feat`: a new feature
- `fix`: a bug fix
- `docs`: documentation only
- `test`: adding or changing tests
- `refactor`: a change that does not alter behaviour
- `chore`: build, tooling, or other housekeeping

## Commit messages

Format commits as `<type>: <message>`, for example `feat: add caseof keyword`.

- Write the message in the imperative mood: "add keyword", not "added keyword".
- Keep the first line under about 72 characters. Add a body if the reason for the change isn't obvious.

## Coding standards

- Match the style of the surrounding code: naming, formatting, and comment density.
- Keep code readable and comments short. Explain why, not what.
- Code should build without compiler warnings.
- Don't commit build output, virtual environments, or editor files.

## Tests

- Add or update tests for any change in behaviour.
- All tests should pass before a pull request is opened.

## Specifications

The files in `specs/` are the source of truth. If a change affects them, update the spec first and make the code follow it.

## Questions

For questions or larger design changes, open an issue before writing code.
