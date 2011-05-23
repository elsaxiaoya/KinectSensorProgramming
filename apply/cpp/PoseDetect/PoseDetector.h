#ifndef POSEDETECTOR_H_INCLUDE
#define POSEDETECTOR_H_INCLUDE

#include "SkeletonJointPosition.h"

class PoseDetector : SkeletonJointPosition
{
public:
  PoseDetector( IplImage* camera, xn::SkeletonCapability& skelton,
    xn::DepthGenerator& depth, XnUserID player )
    :SkeletonJointPosition(camera, skelton, depth, player)
  {
  }

  bool detect()
  {
    bool isCross = CrossHitCheck(
                        GetJointPosition(XN_SKEL_LEFT_ELBOW).position,
                        GetJointPosition(XN_SKEL_LEFT_HAND).position,
                        GetJointPosition(XN_SKEL_RIGHT_ELBOW).position,
                        GetJointPosition(XN_SKEL_RIGHT_HAND).position);
    if (isCross) {
      XnPoint3D point = GetCrossPoint(
                        GetJointPosition(XN_SKEL_LEFT_ELBOW).position,
                        GetJointPosition(XN_SKEL_LEFT_HAND).position,
                        GetJointPosition(XN_SKEL_RIGHT_ELBOW).position,
                        GetJointPosition(XN_SKEL_RIGHT_HAND).position);

      depth_.ConvertRealWorldToProjective(1, &point, &point);
      ::cvCircle(camera_, cvPoint(point.X, point.Y), 10, cvScalar(255, 0, 0), 10);
    }

    return isCross;
  }
    
  // ������̃y�[�W���Q�l�ɂ��܂���
  // http://net2.cocolog-nifty.com/blog/2009/11/post-8792.html
  static bool CrossHitCheck(XnVector3D a1, XnVector3D a2, XnVector3D b1, XnVector3D b2)
  {
    // �O��:axb = ax*by - ay*bx
    // �O�ςƎg�p���Č���������s�Ȃ�
    double v1 = (a2.X - a1.X) * (b1.Y - a1.Y) - (a2.Y - a1.Y) * (b1.X - a1.X);
    double v2 = (a2.X - a1.X) * (b2.Y - a1.Y) - (a2.Y - a1.Y) * (b2.X - a1.X);
    double m1 = (b2.X - b1.X) * (a1.Y - b1.Y) - (b2.Y - b1.Y) * (a1.X - b1.X);
    double m2 = (b2.X - b1.X) * (a2.Y - b1.Y) - (b2.Y - b1.Y) * (a2.X - b1.X);
    // +-,-+��������-�l�ɂȂ�̂ł��ꂼ����|���Ċm�F����
    if((v1*v2 <= 0) && (m1*m2 <= 0)){
      return true; // �Q�Ƃ����E�ɂ�����
    }else{
      return false;
    }
  }

  // ������̃y�[�W���Q�l�ɂ��܂���
  // http://net2.cocolog-nifty.com/blog/2009/11/post-dbea.html
  static XnPoint3D GetCrossPoint(XnVector3D a1, XnVector3D a2, XnVector3D b1, XnVector3D b2)
  {
    XnPoint3D tmp;// �v�Z�p
    // �P�߂̎�
    float v1a = (a1.Y - a2.Y) / (a1.X - a2.X);
    float v1b = (a1.X*a2.Y - a1.Y*a2.X) / (a1.X - a2.X);
    // �Q�߂̎�
    float v2a = (b1.Y - b2.Y) / (b1.X - b2.X);
    float v2b = (b1.X*b2.Y - b1.Y*b2.X) / (b1.X - b2.X);
    // �ŏI�I�Ȍ�_
    tmp.X = (v2b-v1b) / (v1a-v2a);
    tmp.Y = v1a * tmp.X + v1b;
    return tmp;// x,y�̓�����Ԃ�
  }
};

#endif // #ifndef POSEDETECTOR_H_INCLUDE
