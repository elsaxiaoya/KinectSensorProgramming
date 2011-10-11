﻿using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using OpenNI;

namespace Player
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";
    private const string RECORD_PATH = @"../../../../../Data/record.oni";

    private Context context;
    private ImageGenerator image;
    private DepthGenerator depth;
    private OpenNI.Player player;

    private bool isShowImage = true;
    private bool isShowDepth = true;

    private int[] histogram;

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      context = new Context();
      // OpenNI 1.3.2.1 : OpenFileRecordingExを使うように促されるが、使用するとアクセスバイオレーションになる
      // OpenNI 1.3.4.3 : OpenFileRecordingExを使うと起動しない
      context.OpenFileRecording(RECORD_PATH);

      // プレーヤーの作成
      player = context.FindExistingNode(NodeType.Player) as OpenNI.Player;
      if (player == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // 終端に達したら通知するコールバックを登録する
      player.EndOfFileReached += new EventHandler(player_EndOfFileReached);

      // イメージジェネレータの作成
      image = context.FindExistingNode(NodeType.Image) as ImageGenerator;
      if (image == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // デプスジェネレータの作成
      depth = context.FindExistingNode(NodeType.Depth) as DepthGenerator;
      if (depth == null) {
        throw new Exception(context.GlobalErrorState);
      }
      
      // ヒストグラムバッファの作成
      histogram = new int[depth.DeviceMaxDepth];
    }

    // 描画
    private unsafe void xnDraw()
    {
      // ノードの更新を待ち、データを取得する
      context.WaitAndUpdateAll();
      ImageMetaData imageMD = image.GetMetaData();
      DepthMetaData depthMD = depth.GetMetaData();

      CalcHist(depthMD);

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                             System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.ImageMapPtr.ToPointer();
        ushort* dep = (ushort*)depth.DepthMapPtr.ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3, ++dep) {
          byte pixel = (byte)histogram[*dep];
          // ヒストグラムの対象外か、デプスを表示しない場合
          if (pixel == 0 || !isShowDepth) {
            // イメージを表示する場合は、カメラ画像をコピーする
            if (isShowImage) {
              dst[0] = src[2];
              dst[1] = src[1];
              dst[2] = src[0];
            }
            // イメージを描画しない場合は、白にする
            else {
              dst[0] = 255;
              dst[1] = 255;
              dst[2] = 255;
            }
          }
          // それ以外の場所はヒストラムを描画
          else {
            dst[0] = 0;
            dst[1] = pixel;
            dst[2] = pixel;
          }
        }

        bitmap.UnlockBits(data);
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
      // 反転する
      if (key == Keys.M) {
        context.GlobalMirror = !context.GlobalMirror;
      }
      // 画像の表示/非表示
      else if (key == Keys.I) {
        isShowImage = !isShowImage;
      }
      // デプスの表示/非表示
      else if (key == Keys.D) {
        isShowDepth = !isShowDepth;
      }
    }

    // 記録の終了を通知する
    void player_EndOfFileReached(object sender, EventArgs e)
    {
    }

    // ヒストグラムの計算
    private unsafe void CalcHist(DepthMetaData depthMD)
    {
      for (int i = 0; i < histogram.Length; ++i) {
        histogram[i] = 0;
      }

      ushort* pDepth = (ushort*)depthMD.DepthMapPtr.ToPointer();

      int points = 0;
      for (int y = 0; y < depthMD.YRes; ++y) {
        for (int x = 0; x < depthMD.XRes; ++x, ++pDepth) {
          ushort depthVal = *pDepth;
          if (depthVal != 0) {
            histogram[depthVal]++;
            points++;
          }
        }
      }

      for (int i = 1; i < histogram.Length; i++) {
        histogram[i] += histogram[i - 1];
      }

      if (points > 0) {
        for (int i = 1; i < histogram.Length; i++) {
          histogram[i] = (int)(256 * (1.0f - (histogram[i] / (float)points)));
        }
      }
    }
  }
}
