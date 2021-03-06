﻿using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Diagnostics;
using OpenNI;

namespace Gesture
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private Context context;
    private ImageGenerator image;
    private DepthGenerator depth;
    private GestureGenerator gesture;

    private string[] gestures;
    private int gestureIndex = 0;

    // ジェスチャーの状態
    enum GestureStatus
    {
      Unrecognize,    // 未検出
      Progress,       // 検出中
      Recognized,     // 検出した
    }

    private GestureStatus gestureStatus = GestureStatus.Unrecognize;

    // 描画用
    private Pen pen = new Pen(Color.Red, 5);
    private Brush brush = new SolidBrush(Color.Black);
    private Font font = new Font("Arial", 30);
    private PointF point = new PointF(0, 0);

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      ScriptNode scriptNode;
      context = Context.CreateFromXmlFile( CONFIG_XML_PATH, out scriptNode );

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

      // デプスの座標をイメージに合わせる
      depth.AlternativeViewpointCapability.SetViewpoint(image);

      // ジェスチャージェネレータの作成
      gesture = context.FindExistingNode(NodeType.Gesture) as GestureGenerator;
      if (depth == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // ジェスチャーの作成と登録
      gestures = gesture.EnumerateAllGestures();
      gesture.AddGesture(gestures[gestureIndex]);
      string[] activeGestures = gesture.GetAllActiveGestures();

      // ジェスチャーの機能確認
      foreach (string name in gestures) {
        Trace.WriteLine(name + ":" +
          "Available:" + gesture.IsGestureAvailable(name) + 
          " ProgressSupported:" + gesture.IsGestureProgressSupported(name));
      }

      // ジェスチャー用のコールバックを登録
      gesture.GestureRecognized += new EventHandler<GestureRecognizedEventArgs>(gesture_GestureRecognized);
      gesture.GestureProgress += new EventHandler<GestureProgressEventArgs>(gesture_GestureProgress);
      gesture.GestureChanged += new EventHandler(gesture_GestureChanged);

      // ジェスチャーの検出開始
      context.StartGeneratingAll();
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);
      ImageMetaData imageMD = image.GetMetaData();

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                        System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.ImageMapPtr.ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3) {
          dst[0] = src[2];
          dst[1] = src[1];
          dst[2] = src[0];
        }

        bitmap.UnlockBits(data);
      }


      // 現在の状態を表示する
      Graphics g = Graphics.FromImage(bitmap);
      string message = "Gesture:" + gestures[gestureIndex] +
                      ", Status:" + gestureStatus.ToString() + "\n";
      g.DrawString(message, font, brush, point);
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
      // ジェスチャーの変更
      if (key == Keys.G) {
        gesture.StopGenerating();
        gesture.RemoveGesture(gestures[gestureIndex]);
        ++gestureIndex;
        if (gestureIndex >= gestures.Length) {
          gestureIndex = 0;
        }
        gesture.AddGesture(gestures[gestureIndex]);
        gesture.StartGenerating();
      }
    }

    // ジェスチャーの検出中
    void gesture_GestureProgress(object sender, GestureProgressEventArgs e)
    {
      gestureStatus = GestureStatus.Progress;
      Trace.WriteLine(e.Gesture + ":" + e.Progress);
    }

    // ジェスチャーを検出した
    void gesture_GestureRecognized(object sender, GestureRecognizedEventArgs e)
    {
      gestureStatus = GestureStatus.Recognized;
    }

    // ジェスチャーが変更された
    void gesture_GestureChanged(object sender, EventArgs e)
    {
      gestureStatus = GestureStatus.Unrecognize;
    }
  }
}
