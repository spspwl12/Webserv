# 웹서버 프로젝트

## 프로젝트 소개
이 프로젝트는 소켓을 활용한 실시간 채팅 기능, 파일 공유 기능을 갖춘 초경량 포터블 웹서버입니다.<br>
아파치 웹서버의 기본 파일 브라우저 UI를 참고해 제작했습니다.<br>
  
이 프로그램에 사용된 Third-Party Program 은 다음과 같습니다.
- [**zlib**](https://zlib.net) → 정적 라이브러리, binary로 zip 압축 해제하는 함수 추가해서 사용합니다.<br>

## 주요 기능
- HTTP 범위 요청(HTTP Range Requests) 지원.
- 웹소켓 채팅: 사용자 간 실시간 메시지 전송. 간단한 문자열 공유 가능
- 파일 업로드: 진행바를 추가하여 업로드 상태 확인 가능.
- 다양한 MIME types 지원

## 컴파일 및 실행 방법
1. Visual Studio 2022 를 [다운로드](https://visualstudio.microsoft.com/ko/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022) 합니다.<br>
2. Visual Studio Installer 가 뜨면 <br>
![image](https://github.com/user-attachments/assets/1a041214-bb2a-40d8-a113-f55b0b1a441f) 와 같이 체크를 합니다.<br>
3. 설치 (Install)를 합니다.<br>
4. 프로젝트를 다운로드 합니다.<br>
![image](https://github.com/user-attachments/assets/f2daac49-78f8-481a-9659-af82b2057f59)
5. 압축을 풉니다.<br>
6. Webserv.sln 파일을 더블클릭 해 프로젝트를 엽니다.<br>
7. 플랫폼을 x86 그리고 구성을 Release 로 설정합니다.<br>
![image](https://github.com/user-attachments/assets/7226232f-2a41-4020-a3c1-3da166fd208e)
8. F7를 눌러 컴파일 합니다.<br>
9. 컴파일이 완료되면, 프로젝트 폴더안의 Build 폴더에 Webserv.exe 를 실행합니다.<br>
10. Server Address에 "127.0.0.1" 를 선택하고 다운로드 폴더에 공유하고 싶은 폴더 경로를 입력 후 Start 버튼을 누릅니다.<br>
![{F226E0A1-771D-4FE9-A030-6B61F03BBB8E}](https://github.com/user-attachments/assets/fea4ebd8-c6af-4ecf-86bf-7e94a8e863ac)
11. http://127.0.0.1:80/ 에 들어가서 확인합니다.<br>
![{60AAC508-8A1E-4666-ABD4-41423C3829F2}](https://github.com/user-attachments/assets/55efc268-dc27-4b5c-9898-e44f71e952b6)

## 특이사항
- 웹 서버 구동에 필요한 리소스 파일은 resource\pack.zip 에 정의되어 있습니다.

## 작동 화면 
### 조작
![Animation7](https://github.com/user-attachments/assets/9504fbea-7208-41b5-b3f6-099441847f9a)
### 파일 다운로드
![Animation1](https://github.com/user-attachments/assets/f9fc6b25-a15d-4786-9a3c-6622ddea56ba)
### 파일 크롬에서 열기 ( HTTP Range Requests 포함 )
![Animation2](https://github.com/user-attachments/assets/cebf413e-d2cd-434d-a313-acfa95ddab19)
### 폴더 브라우징
![Animation21](https://github.com/user-attachments/assets/8cb36ba6-7303-403b-ba59-da8312b27ad5)
### 업로드
![Animation4](https://github.com/user-attachments/assets/dfff2fb3-3560-408b-a4c3-a23b07e6ca7f)
### 웹소켓 채팅
![Animation5](https://github.com/user-attachments/assets/3b737056-a8de-4d98-8199-43171004fd19)
