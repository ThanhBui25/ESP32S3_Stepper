# Quy tắc bắt buộc cho Agent (Agent Operating Guidelines)

## 1. QUY TẮC BẮT BUỘC: HỎI VÀ XÁC NHẬN TRƯỚC KHI XÓA FILE / THƯ MỤC
- **Tuyệt đối KHÔNG tự ý xóa file/thư mục**: Dù là lệnh Terminal (`rm`, `del`, `Remove-Item`, `rmdir`, `git clean`, v.v.) hay thông qua bất kỳ công cụ nào, Agent **BẮT BUỘC** phải dừng lại và xin phép người dùng trước khi xóa bất kỳ file hoặc thư mục nào.
- **Quy trình xin phép khi cần xóa**:
  1. Liệt kê rõ ràng danh sách tên đường dẫn các file / thư mục dự định xóa.
  2. Giải thích ngắn gọn lý do vì sao cần xóa.
  3. Dừng lại, hỏi ý kiến và chờ người dùng gõ xác nhận đồng ý (ví dụ: "đồng ý", "yes", "ok") mới được phép tiến hành xóa.

## 2. TỰ ĐỘNG THỰC HIỆN CÁC TÁC VỤ KHÁC (KHÔNG LÀM PHIỀN NGƯỜI DÙNG)
- Đối với các thao tác an toàn và xây dựng như:
  - Đọc tài liệu, tra cứu web, đọc URL.
  - Xem nội dung file, tìm kiếm mã nguồn.
  - Tạo file mới, chỉnh sửa / cập nhật nội dung file.
  - Chạy lệnh build, test, lint, cài đặt package, clone repository.
  -> Agent chủ động tự động thực hiện mà **KHÔNG CẦN** hỏi xác nhận từng bước, giúp tiết kiệm thời gian cho người dùng.
