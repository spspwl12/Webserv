# 웹서버 프로젝트 (C 언어 기반)

## 프로젝트 소개
이 프로젝트는 소켓을 활용한 실시간 채팅 기능, 파일 공유 기능을 갖춘 초경량 포터블 웹서버입니다.<br>
아파치 웹서버의 기본 파일 브라우저 UI를 참고해 제작했습니다.

## 주요 기능
- HTTP 범위 요청(HTTP Range Requests) 지원.
- 웹소켓 채팅: 사용자 간 실시간 메시지 전송. 간단한 문자열 공유 가능
- 파일 업로드: 진행바를 추가하여 업로드 상태 확인 가능.
- 다양한 MIME types 지원

## 컴파일 방법 ( zlib.lib )
( zlib.lib; 해당 라이브러리는 이미 컴파일 되어 zlib\bin 에 동봉되어 있습니다., zlib.lib 의 바이러스토탈 진단결과는 하단 참조 )
1. Visual Studio 2022 를 다운로드 합니다. ( https://visualstudio.microsoft.com/ko/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030&passive=false )
2. Visual Studio Installer 가 뜨면 
![image](https://github.com/user-attachments/assets/1a041214-bb2a-40d8-a113-f55b0b1a441f)
와 같이 체크를 합니다.
3. 설치 (Install)를 합니다.
4. 프로젝트를 다운로드 합니다.
![image](https://github.com/user-attachments/assets/f2daac49-78f8-481a-9659-af82b2057f59)
5. 압축을 풉니다.
6. Webserv\zlib 으로 들어갑니다.
7. zlib.sln 파일을 더블클릭 해 프로젝트를 엽니다.
8. 플랫폼을 x86 그리고 구성을 Release 로 설정합니다.
![image](https://github.com/user-attachments/assets/18b5ea10-94f5-457c-887b-3a4902dbae23)
9. F7를 눌러 컴파일 합니다.
10. 컴파일이 완료되면, Webserv\zlib\bin 폴더 안에 zlib.lib 파일이 있는지 확인합니다. 없다면 8 과정부터 다시 시도합니다.

## 컴파일 및 실행 방법 ( Webserv.exe )
1. Visual Studio 2022 를 다운로드 합니다. ( https://visualstudio.microsoft.com/ko/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030&passive=false )
2. Visual Studio Installer 가 뜨면 
![image](https://github.com/user-attachments/assets/1a041214-bb2a-40d8-a113-f55b0b1a441f) 와 같이 체크를 합니다.
3. 설치 (Install)를 합니다.
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

## 사용 라이브러리
- zlib ( https://zlib.net ) => 정적 라이브러리, binary로 zip 압축 해제하는 함수 추가해서 사용<br>
![image](https://github.com/user-attachments/assets/ea7c0ffe-9fa8-413f-95cc-921de53c0add)

## 특이사항
- 웹 서버 구동에 필요한 리소스 파일은 resource\pack.zip 에 정의되어 있습니다.

## zlib.lib의 바이러스토탈 결과
- zlib.lib ( https://www.virustotal.com/gui/file/b19ae2480325fc5b652a7d14fe233d861fc53864910aa93575fcc554c29ad34e )
  ![x86](https://github.com/user-attachments/assets/c1061b50-7fe6-4368-b34b-a02e95f8269c)
- zlib_x64.lib ( https://www.virustotal.com/gui/file/d3e8f0c4c13f361db3d925f7efae495ab30641f8c58c0cbfe19616a6e8b8de64 )
  ![x64](https://github.com/user-attachments/assets/8a03b03a-0281-4341-b5b9-77c78d6de301)
- zlibd.lib ( https://www.virustotal.com/gui/file/840a6891a14a528fdd2745cfe148f3cee3d78dbc3fea75f55923a45bb1ccf5e0 )
  ![x86](https://github.com/user-attachments/assets/83889015-8cea-4510-98cf-8b06eb153be0)
- zlibd_x64.lib ( https://www.virustotal.com/gui/file/79d32aabd5335c3e3adac77e98daf25446fdb31a3ba04307b3b5ced4884eb7b8 )
  ![x64](https://github.com/user-attachments/assets/8748bb92-43a3-499a-9a52-cb68b1d400ac)


