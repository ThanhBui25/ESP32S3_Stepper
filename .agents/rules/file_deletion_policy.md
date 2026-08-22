---
trigger: always_on
description: Quy định bắt buộc xác nhận trước khi xóa file hoặc thư mục
---

# File Deletion Safety Policy

1. **Strict Deletion Protection**:
   - The agent MUST NEVER delete any file or directory without explicit confirmation from the user.
   - Prohibited actions without prior user approval include `rm`, `del`, `Remove-Item`, `git clean`, deleting via file system tools, or emptying directories.

2. **Confirmation Protocol**:
   - Clearly state the files/directories to be deleted.
   - Explain the reason for deletion.
   - Ask for confirmation and STOP tool execution until the user replies.

3. **Autonomous Non-Destructive Actions**:
   - Reading URLs, browsing, creating/modifying files, running builds, tests, and non-destructive terminal commands should proceed autonomously without unnecessary permission prompts.
