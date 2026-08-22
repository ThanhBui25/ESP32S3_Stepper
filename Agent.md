# Quy định hoạt động của Agent (Agent Rules)

## 1. Quy định về việc xóa File và Thư mục (File Deletion Policy)

- **Bắt buộc xác nhận trước khi xóa**: Tuyệt đối **KHÔNG ĐƯỢC** tự ý xóa bất kỳ file hoặc thư mục nào trong dự án mà chưa có sự xác nhận rõ ràng từ người dùng.
- **Quy trình khi cần xóa file/thư mục**:
  1. Liệt kê rõ ràng danh sách các file/thư mục dự định xóa.
  2. Nêu rõ lý do cần xóa và các tác động/rủi ro tiềm ẩn (nếu có).
  3. Dừng lại, hỏi ý kiến người dùng và chờ phản hồi xác nhận (đồng ý xóa) từ người dùng trước khi thực thi bất kỳ lệnh hoặc thao tác xóa nào.
- **Không thực thi ngầm**: Không dùng các script hoặc lệnh tự động xóa (ví dụ: `rm`, `del`, `Remove-Item`, clean script, ...) nếu chưa thông báo và được chấp thuận.
