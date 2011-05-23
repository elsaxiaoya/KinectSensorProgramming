#include <iostream>
#include <stdexcept>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <XnCppWrapper.h>
#include <XnVSessionManager.h>

// 設定ファイルのパス(環境に合わせて変更してください)
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
#elif (XN_PLATFORM == XN_PLATFORM_MACOSX)
const char* CONFIG_XML_PATH = "../../../../../Data/SamplesConfig.xml";
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
#else
const char* CONFIG_XML_PATH = "Data/SamplesConfig.xml";
#endif


// セッション開始の検出を通知されるコールバック
void XN_CALLBACK_TYPE SessionDetected(const XnChar* strFocus,
      const XnPoint3D& ptPosition, XnFloat fProgress, void* UserCxt)
{
  std::cout << "SessionDetected:" << strFocus << "," << fProgress << std::endl;
}

// セッションの開始を通知されるコールバック
void XN_CALLBACK_TYPE SessionStart(const XnPoint3D& pFocus, void* UserCxt)
{
  std::cout << "SessionStart" << std::endl;
}

// セッションの終了を通知されるコールバック
void XN_CALLBACK_TYPE SessionEnd(void* UserCxt)
{
  std::cout << "SessionEnd" << std::endl;
}

int main (int argc, char * const argv[]) {
  IplImage* camera = 0;

  try {
    // OpenNIのコンテキストを初期化する
    xn::Context context;
    XnStatus rc = context.InitFromXmlFile(CONFIG_XML_PATH);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }        

    // イメージジェネレータの作成
    xn::ImageGenerator image;
    rc = context.FindExistingNode(XN_NODE_TYPE_IMAGE, image);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // カメラサイズのイメージを作成(8bitのRGB)
    XnMapOutputMode outputMode;
    image.GetMapOutputMode(outputMode);
    camera = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
      IPL_DEPTH_8U, 3);
    if (!camera) {
      throw std::runtime_error("error : cvCreateImage");
    }


    // NITEのセッションマネージャーを初期化する
    XnVSessionManager sessionManager;
    rc = sessionManager.Initialize(&context, "Wave,Click", "RaiseHand");
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // セッションの開始終了を通知するコールバックを登録する
    XnVHandle sessionCallnack = sessionManager.RegisterSession(
      0, &SessionStart, &SessionEnd,
      &SessionDetected);

    // メインループ
    while (1) {
      // それぞれのデータを更新する
      context.WaitAndUpdateAll();
      sessionManager.Update(&context);

      xn::ImageMetaData imageMD;
      image.GetMetaData(imageMD);

      // カメラ画像の表示
      memcpy(camera->imageData, imageMD.RGB24Data(), camera->imageSize);
      ::cvCvtColor(camera, camera, CV_BGR2RGB);
      ::cvShowImage("KinectImage", camera);

      // キーの取得
      char key = cvWaitKey(10);
      // 終了する
      if (key == 'q') {
        break;
      }
    }

    // セッションのコールバックを削除する
    sessionManager.UnregisterSession(sessionCallnack);
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }

  ::cvReleaseImage(&camera);

  return 0;
}
