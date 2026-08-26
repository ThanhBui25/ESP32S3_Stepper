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
  - Tạo file mới, chỉnh sửa / cập nhật nội dung file trên máy cục bộ.
  - Chạy lệnh build, test, lint, cài đặt package, nạp code xuống mạch.
  -> Agent chủ động tự động thực hiện mà **KHÔNG CẦN** hỏi xác nhận từng bước, giúp tiết kiệm thời gian cho người dùng.

## 3. QUY TẮC BẢO VỆ GITHUB (KHÔNG TỰ Ý ĐẨY CODE LÊN GITHUB)
- **Tuyệt đối KHÔNG tự ý gửi/đẩy code lên GitHub (`git push`)**: Agent chỉ thực hiện đẩy code lên GitHub khi người dùng ra lệnh rõ ràng (ví dụ: *"đẩy lên github"*, *"push code"*, *"lưu lên github"*).

## 4. QUY TẮC BẮT BUỘC: KIỂM TRA AN TOÀN & TƯƠNG THÍCH PHẦN CỨNG (HARDWARE COMPATIBILITY CHECK)
- Khi người dùng đề cập, hỏi hoặc muốn thêm/ghép nối bất kỳ **thiết bị, linh kiện mới nào** (MCU, Driver, Động cơ, Encoder, Cảm biến, Nguồn cấp, Module...):
  1. Agent **BẮT BUỘC** phải tự động đối chiếu thông số điện áp logic, dòng điện, số pha, cách ly tín hiệu với các thiết bị đang được sử dụng trong hệ thống (`HARDWARE_DATABASE.md` & `WIRING.md`).
  2. Nếu phát hiện **KHÔNG TƯƠNG THÍCH**, **SAI ĐIỆN ÁP**, hoặc **CÓ NGUY CƠ GÂY CHÁY NỔ/HỎNG LINH KIỆN**:
     - Agent **BẮT BUỘC PHẢI BẬT CẢNH BÁO NGAY LẬP TỨC**:
       ```markdown
       ⛔ CẢNH BÁO NGUY HIỂM: [TÊN THIẾT BỊ] KHÔNG ĐƯỢC SỬ DỤNG TRỰC TIẾP!
       ```
     - Giải thích rõ nguyên nhân kỹ thuật gây hỏng hóc.
     - Hướng dẫn điều kiện bắt buộc nếu muốn sử dụng (ví dụ: cần thêm mạch đệm Level Shifter, Opto cách ly, tụ đệm, trở phân áp...) hoặc đề xuất thiết bị thay thế an toàn.

