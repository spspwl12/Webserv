# 웹서버 프로젝트 (C 언어 기반)

프로젝트 소개
이 프로젝트는 **웹소켓(websocket)** 을 활용한 실시간 채팅 기능, 업로드 및 다운로드 기능을 갖춘 웹서버입니다. 
아파치 웹서버의 기본 파일 브라우저 UI를 참고해 제작했습니다.

주요 기능
- HTTP 범위 요청(HTTP Range Requests) 지원.
- 웹소켓 채팅: 사용자 간 실시간 메시지 전송. 간단한 문자열 공유 가능
- 파일 업로드: 진행바를 추가하여 업로드 상태 확인 가능.
- 다양한 MIME types 지원

컴파일 및 실행 방법
1. Visual Studio 2022 를 다운로드 합니다. ( https://visualstudio.microsoft.com/ko/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030&passive=false )
2. Visual Studio Installer 가 뜨면 
![image](https://github.com/user-attachments/assets/1a041214-bb2a-40d8-a113-f55b0b1a441f)
와 같이 체크를 합니다.
3. 설치 (Install)를 합니다
4. 프로젝트를 다운로드 합니다.
![image](https://github.com/user-attachments/assets/f2daac49-78f8-481a-9659-af82b2057f59)
5. 압축을 풉니다.
6. Webserv.sln 파일을 더블클릭 해 프로젝트를 엽니다.
7. 플랫폼을 x86 그리고 구성을 Release 로 설정합니다.
![image](https://github.com/user-attachments/assets/f7f2ad02-3bc2-43ca-9b05-4c9188ee7b6d)
8. F7를 눌러 컴파일 합니다.
9. 컴파일이 완료되면, 프로젝트 폴더안의 Build 폴더에 Webserv.exe 를 실행합니다.
10. Server Address에 "127.0.0.1" 를 선택하고 다운로드 폴더에 공유하고 싶은 폴더 경로를 입력 후 Start 버튼을 누릅니다.
![{F226E0A1-771D-4FE9-A030-6B61F03BBB8E}](https://github.com/user-attachments/assets/fea4ebd8-c6af-4ecf-86bf-7e94a8e863ac)
11. http://127.0.0.1:80/ 에 들어가서 확인합니다.
![{60AAC508-8A1E-4666-ABD4-41423C3829F2}](https://github.com/user-attachments/assets/55efc268-dc27-4b5c-9898-e44f71e952b6)
