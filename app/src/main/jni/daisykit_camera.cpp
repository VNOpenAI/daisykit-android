#include "daisykit_camera.h"

ncnn::Mutex g_lock;
static DemoApp current_demo = DemoApp::kDemoPushUpCounter;
static daisykit::flows::PushupCounterFlow* pushup_flow;
static daisykit::flows::BarcodeScannerFlow* barcode_flow;

class DaisykitCam : public NdkCameraWindow {
 public:
  virtual void on_image_render(cv::Mat& rgb) const;
};

void DaisykitCam::on_image_render(cv::Mat& rgb) const {
  {
    ncnn::MutexLockGuard g(g_lock);
    switch (current_demo) {
      case DemoApp::kDemoPushUpCounter:
        if (!pushup_flow) return;
        pushup_flow->Process(rgb);
        pushup_flow->DrawResult(rgb);
        break;
      case DemoApp::kDemoBarcodeReader:
        if (!barcode_flow) return;
        std::string result = barcode_flow->Process(rgb, true);
        break;
    }
  }
}

static DaisykitCam* g_camera = nullptr;

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnLoad");

  g_camera = new DaisykitCam;

  return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
  __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnUnload");

  {
    ncnn::MutexLockGuard g(g_lock);
    delete pushup_flow;
    pushup_flow = nullptr;
    delete barcode_flow;
    barcode_flow = nullptr;
  }

  delete g_camera;
  g_camera = nullptr;
}

// public native boolean loadModel(AssetManager mgr, int demoid, int cpugpu);
JNIEXPORT jboolean JNICALL Java_com_vnopenai_daisykit_DaisykitCamera_loadDemo(
    JNIEnv* env, jobject thiz, jobject assetManager, jint demoid, jint cpugpu) {
  AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

  {
    ncnn::MutexLockGuard g(g_lock);
    current_demo = static_cast<DemoApp>(demoid);
    std::string config;
    switch (current_demo) {
      case DemoApp::kDemoPushUpCounter:
        config =
            read_file_from_assets(mgr, "configs/pushup_counter_config.json");
        pushup_flow = new daisykit::flows::PushupCounterFlow(mgr, config);
        break;
      case DemoApp::kDemoBarcodeReader:
        config =
            read_file_from_assets(mgr, "configs/barcode_scanner_config.json");
        barcode_flow = new daisykit::flows::BarcodeScannerFlow(mgr, config);
        break;
    }
  }

  return JNI_TRUE;
}

// public native boolean openCamera(int facing);
JNIEXPORT jboolean JNICALL Java_com_vnopenai_daisykit_DaisykitCamera_openCamera(
    JNIEnv* env, jobject thiz, jint facing) {
  if (facing < 0 || facing > 1) return JNI_FALSE;

  __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "openCamera %d", facing);

  g_camera->open((int)facing);

  return JNI_TRUE;
}

// public native boolean closeCamera();
JNIEXPORT jboolean JNICALL
Java_com_vnopenai_daisykit_DaisykitCamera_closeCamera(JNIEnv* env,
                                                      jobject thiz) {
  __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "closeCamera");

  g_camera->close();

  return JNI_TRUE;
}

// public native boolean setOutputWindow(Surface surface);
JNIEXPORT jboolean JNICALL
Java_com_vnopenai_daisykit_DaisykitCamera_setOutputWindow(JNIEnv* env,
                                                          jobject thiz,
                                                          jobject surface) {
  ANativeWindow* win = ANativeWindow_fromSurface(env, surface);

  __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setOutputWindow %p", win);

  g_camera->set_window(win);

  return JNI_TRUE;
}
}
