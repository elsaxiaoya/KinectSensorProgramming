﻿// Windows の場合はReleaseコンパイルにすると
// 現実的な速度で動作します
#include <iostream>
#include <stdexcept>
#include <vector>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <XnCppWrapper.h>

#include "SkeltonDrawer.h"
#include "PoseDetector.h"

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

// ユーザーの色づけ
const XnFloat Colors[][3] =
{
  {1,1,1},    // ユーザーなし
  {0,1,1},  {0,0,1},  {0,1,0},
  {1,1,0},  {1,0,0},  {1,.5,0},
  {.5,1,0}, {0,.5,1}, {.5,0,1},
  {1,1,.5},
};

// ユーザー検出
void XN_CALLBACK_TYPE UserDetected(xn::UserGenerator& generator,
  XnUserID nId, void* pCookie)
{
  std::cout << "ユーザー検出:" << nId << " " <<
    generator.GetNumberOfUsers() << "人目" << std::endl;

  XnChar* pose = (XnChar*)pCookie;
  if (pose[0] != '¥0') {
    generator.GetPoseDetectionCap().StartPoseDetection(pose, nId);
  }
  else {
    generator.GetSkeletonCap().RequestCalibration(nId, TRUE);
  }
}

// ユーザー消失
void XN_CALLBACK_TYPE UserLost(xn::UserGenerator& generator,
  XnUserID nId, void* pCookie)
{
  std::cout << "ユーザー消失:" << nId << std::endl;
}

// ポーズ検出
void XN_CALLBACK_TYPE PoseDetected(xn::PoseDetectionCapability& capability,
  const XnChar* strPose, XnUserID nId, void* pCookie)
{
  std::cout << "ポーズ検出:" << strPose << " ユーザー:" << nId << std::endl;

  xn::UserGenerator* user = (xn::UserGenerator*)pCookie;
  user->GetPoseDetectionCap().StopPoseDetection(nId);
  user->GetSkeletonCap().RequestCalibration(nId, TRUE);
}

// ポーズ消失
void XN_CALLBACK_TYPE PoseLost(xn::PoseDetectionCapability& capability,
  const XnChar* strPose, XnUserID nId, void* pCookie)
{
  std::cout << "ポーズ消失:" << strPose << " ユーザー:" << nId << std::endl;
}

// キャリブレーションの開始
void XN_CALLBACK_TYPE CalibrationStart(xn::SkeletonCapability& capability,
  XnUserID nId, void* pCookie)
{
  std::cout << "キャリブレーション開始。ユーザー:" << nId << std::endl;
}

// キャリブレーションの終了
void XN_CALLBACK_TYPE CalibrationEnd(xn::SkeletonCapability& capability,
  XnUserID nId, XnBool bSuccess, void* pCookie)
{
  xn::UserGenerator* user = (xn::UserGenerator*)pCookie;

  // キャリブレーション成功
  if (bSuccess) {
    std::cout << "キャリブレーション成功。ユーザー:" << nId << std::endl;
    user->GetSkeletonCap().StartTracking(nId);
  }
  // キャリブレーション失敗
  else {
    std::cout << "キャリブレーション失敗。ユーザー:" << nId << std::endl;
  }
}

// RGBピクセルの初期化
inline XnRGB24Pixel xnRGB24Pixel( int r, int g, int b )
{
  XnRGB24Pixel pixel = { r, g, b };
  return pixel;
}

int main (int argc, char * argv[])
{
  IplImage* camera = 0;

  try {
    // コンテキストの初期化
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

    // デプスジェネレータの作成
    xn::DepthGenerator depth;
    rc = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depth);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // デプスの座標をイメージに合わせる
    depth.GetAlternativeViewPointCap().SetViewPoint(image);

    // ユーザーの作成
    xn::UserGenerator user;
    rc = context.FindExistingNode( XN_NODE_TYPE_USER, user );
    if ( rc != XN_STATUS_OK ) {
      rc = user.Create(context);
      if ( rc != XN_STATUS_OK ) {
        throw std::runtime_error( xnGetStatusString( rc ) );
      }
    }

    // ユーザー検出機能をサポートしているか確認
    if (!user.IsCapabilitySupported(XN_CAPABILITY_SKELETON)) {
      throw std::runtime_error("ユーザー検出をサポートしてません");
    }

    XnCallbackHandle userCallbacks, calibrationCallbacks, poseCallbacks;
    XnChar pose[20] = "";

    // キャリブレーションにポーズが必要
    xn::SkeletonCapability skelton = user.GetSkeletonCap();
    if (skelton.NeedPoseForCalibration()) {
      // ポーズ検出のサポートチェック
      if (!user.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)) {
        throw std::runtime_error("ポーズ検出をサポートしてません");
      }

      // キャリブレーションポーズの取得
      skelton.GetCalibrationPose(pose);

      // ポーズ検出のコールバックを登録
      xn::PoseDetectionCapability pose = user.GetPoseDetectionCap();
      pose.RegisterToPoseCallbacks(&::PoseDetected, &::PoseLost,
        &user, poseCallbacks);
    }

    // ユーザー認識のコールバックを登録
    user.RegisterUserCallbacks(&::UserDetected, &::UserLost, pose,
      userCallbacks);

    // キャリブレーションのコールバックを登録
    skelton.RegisterCalibrationCallbacks(&::CalibrationStart, &::CalibrationEnd,
      &user, calibrationCallbacks);

    // ユーザートラッキングで、すべてをトラッキングする
    skelton.SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

    // ジェスチャー検出の開始
    context.StartGeneratingAll();

    // カメラサイズのイメージを作成(8bitのRGB)
    XnMapOutputMode outputMode;
    image.GetMapOutputMode(outputMode);
    camera = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
      IPL_DEPTH_8U, 3);
    if (!camera) {
      throw std::runtime_error("error : cvCreateImage");
    }

    // 表示状態
    bool isShowImage = true;
    bool isShowUser = true;
    bool isShowSkelton = true;

    // メインループ
    while (1) {
      // すべてのノードの更新を待つ
      context.WaitAndUpdateAll();

      // 画像データの取得
      xn::ImageMetaData imageMD;
      image.GetMetaData(imageMD);

      // ユーザーデータの取得
      xn::SceneMetaData sceneMD;
      user.GetUserPixels(0, sceneMD);

      // カメラ画像の表示
      char* dest = camera->imageData;
      const xn::RGB24Map& rgb = imageMD.RGB24Map();
      for (int y = 0; y < imageMD.YRes(); ++y) {
        for (int x = 0; x < imageMD.XRes(); ++x) {
          // ユーザー表示
          XnLabel label = sceneMD(x, y);
          if (!isShowUser) {
            label = 0;
          }

          // カメラ画像の表示
          XnRGB24Pixel pixel = rgb(x, y);
          if (!isShowImage) {
            pixel = xnRGB24Pixel( 255, 255, 255 );
          }

          // 出力先に描画
          dest[0] = pixel.nRed   * Colors[label][0];
          dest[1] = pixel.nGreen * Colors[label][1];
          dest[2] = pixel.nBlue  * Colors[label][2];
          dest += 3;
        }
      }

      // スケルトンの描画
      if (isShowSkelton) {
        XnUserID aUsers[15];
        XnUInt16 nUsers = 15;
        user.GetUsers(aUsers, nUsers);
        for (int i = 0; i < nUsers; ++i) {
          if (skelton.IsTracking(aUsers[i])) {
            SkeltonDrawer skeltonDrawer(camera, skelton,
              depth, aUsers[i]);
            skeltonDrawer.draw();

            PoseDetector pose(camera, skelton, depth, aUsers[i]);
            bool isCross = pose.detect();
          }
        }
      }

      ::cvCvtColor(camera, camera, CV_BGR2RGB);
      ::cvShowImage("KinectImage", camera);

      // キーイベント
      char key = cvWaitKey(10);
      // 終了する
      if (key == 'q') {
        break;
      }
      // 表示する/しないの切り替え
      else if (key == 'i') {
        isShowImage = !isShowImage;
      }
      else if (key == 'u') {
        isShowUser = !isShowUser;
      }
      else if (key == 's') {
        isShowSkelton = !isShowSkelton;
      }
    }
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }

  ::cvReleaseImage(&camera);

  return 0;
}
