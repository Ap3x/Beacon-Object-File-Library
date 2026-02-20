# BOF Template

This folder contains a template for creating new Beacon Object File projects.

## Placeholders

Replace these placeholders when creating a new project:

| Placeholder | Replace With | Example |
|-------------|-------------|---------|
| `__PROJECT_NAME__` | Your project name | `WhoAmI` |
| `__PROJECT_GUID__` | A new GUID (no braces) | `B3A1F2E4-5C6D-7E8F-9A0B-1C2D3E4F5A6B` |
| `__FUNCTION_NAME__` | Your main function name | `WhoAmI` |

## Files to Rename

After copying, rename these files to match your project name:

- `Template.vcxproj` -> `<ProjectName>.vcxproj`
- `Template.vcxproj.filters` -> `<ProjectName>.vcxproj.filters`
- `Template.vcxproj.user` -> `<ProjectName>.vcxproj.user`
