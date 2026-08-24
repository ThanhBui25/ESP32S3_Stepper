---
trigger: always_on
description: Quy định chỉ gửi mã nguồn lên GitHub khi người dùng ra lệnh rõ ràng
---

# GitHub Push Policy

1. **Strict GitHub Push Protection**:
   - Agent **TUYỆT ĐỐI KHÔNG ĐƯỢC TỰ Ý GỬI/PUSH CODE LÊN GITHUB** (`git push`, `gh repo sync`, v.v.) nếu chưa có sự yêu cầu hoặc ra lệnh rõ ràng từ người dùng.
   - Mọi thay đổi mã nguồn chỉ lưu trữ và biên dịch cục bộ (local).

2. **Khi nào được phép gửi lên GitHub**:
   - Chỉ thực hiện `git push` hoặc đẩy code lên GitHub khi người dùng gửi yêu cầu cụ thể như: *"đẩy lên github"*, *"tải lên github"*, *"push code"*, *"lưu lên github"*.
