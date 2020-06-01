#include <string.h>
#include <iostream>

#include "RtAudioCaptureModule.hpp"
#include "rtc_base/checks.h"
#include "rtc_base/location.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "rtc_base/time_utils.h"

// Audio sample value that is high enough that it doesn't occur naturally when
// frames are being faked. E.g. NetEq will not generate this large sample value
// unless it has received an audio frame containing a sample of this value.
// Even simpler buffers would likely just contain audio sample values of 0.
static const int kHighSampleValue = 0;  // was 10000

// Constants here are derived by running VoE using a real ADM.
// The constants correspond to 10ms of mono audio at 44kHz.
static const int kTimePerFrameMs = 1;   // was 10
static const uint8_t kNumberOfChannels = 1;
static const int kSamplesPerSecond = 44000;
static const int kTotalDelayMs = 0;
static const int kClockDriftMs = 0;
static const uint32_t kMaxVolume = 14392;

enum {
  MSG_START_PROCESS,
  MSG_RUN_PROCESS,
};

RTAudioCaptureModule::RTAudioCaptureModule()
  : audio_callback_(nullptr),
    recording_(false),
    playing_(false),
    play_is_initialized_(false),
    rec_is_initialized_(false),
    current_mic_level_(kMaxVolume),
    started_(false),
    next_frame_time_(0),
    frames_received_(0) {}

RTAudioCaptureModule::~RTAudioCaptureModule() {
  if (process_thread_) {
    process_thread_->Stop();
  }
}

rtc::scoped_refptr<RTAudioCaptureModule> RTAudioCaptureModule::Create() {
  rtc::scoped_refptr<RTAudioCaptureModule> capture_module(
    new rtc::RefCountedObject<RTAudioCaptureModule>());
  if (!capture_module->Initialize()) {
    return nullptr;
  }
  return capture_module;
}

int RTAudioCaptureModule::frames_received() const {
  rtc::CritScope cs(&crit_);
  return frames_received_;
}

int32_t RTAudioCaptureModule::ActiveAudioLayer(
  AudioLayer* /*audio_layer*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::RegisterAudioCallback(
  webrtc::AudioTransport* audio_callback) {
  rtc::CritScope cs(&crit_callback_);
  audio_callback_ = audio_callback;
  return 0;
}

int32_t RTAudioCaptureModule::Init() {
  // Initialize is called by the factory method. Safe to ignore this Init call.
  std::cout << "yes" << std::endl;
  //SetSendBuffer(4);
  return 0;
}

int32_t RTAudioCaptureModule::Terminate() {
  // Clean up in the destructor. No action here, just success.
  return 0;
}

bool RTAudioCaptureModule::Initialized() const {
  RTC_NOTREACHED();
  return 0;
}

int16_t RTAudioCaptureModule::PlayoutDevices() {
  RTC_NOTREACHED();
  return 0;
}

int16_t RTAudioCaptureModule::RecordingDevices() {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::PlayoutDeviceName(
  uint16_t /*index*/,
  char /*name*/[webrtc::kAdmMaxDeviceNameSize],
  char /*guid*/[webrtc::kAdmMaxGuidSize]) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::RecordingDeviceName(
  uint16_t /*index*/,
  char /*name*/[webrtc::kAdmMaxDeviceNameSize],
  char /*guid*/[webrtc::kAdmMaxGuidSize]) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::SetPlayoutDevice(uint16_t /*index*/) {
  // No playout device, just playing from file. Return success.
  return 0;
}

int32_t RTAudioCaptureModule::SetPlayoutDevice(WindowsDeviceType /*device*/) {
  if (play_is_initialized_) {
    return -1;
  }
  return 0;
}

int32_t RTAudioCaptureModule::SetRecordingDevice(uint16_t /*index*/) {
  // No recording device, just dropping audio. Return success.
  return 0;
}

int32_t RTAudioCaptureModule::SetRecordingDevice(
  WindowsDeviceType /*device*/) {
  if (rec_is_initialized_) {
    return -1;
  }
  return 0;
}

int32_t RTAudioCaptureModule::PlayoutIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::InitPlayout() {
  play_is_initialized_ = true;
  return 0;
}

bool RTAudioCaptureModule::PlayoutIsInitialized() const {
  return play_is_initialized_;
}

int32_t RTAudioCaptureModule::RecordingIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::InitRecording() {
  rec_is_initialized_ = true;
  return 0;
}

bool RTAudioCaptureModule::RecordingIsInitialized() const {
  return rec_is_initialized_;
}

int32_t RTAudioCaptureModule::StartPlayout() {
  if (!play_is_initialized_) {
    return -1;
  }
  {
    rtc::CritScope cs(&crit_);
    playing_ = true;
  }
  bool start = true;
  UpdateProcessing(start);
  return 0;
}

int32_t RTAudioCaptureModule::StopPlayout() {
  bool start = false;
  {
    rtc::CritScope cs(&crit_);
    playing_ = false;
    start = ShouldStartProcessing();
  }
  UpdateProcessing(start);
  return 0;
}

bool RTAudioCaptureModule::Playing() const {
  rtc::CritScope cs(&crit_);
  return playing_;
}

int32_t RTAudioCaptureModule::StartRecording() {
  if (!rec_is_initialized_) {
    return -1;
  }
  {
    rtc::CritScope cs(&crit_);
    recording_ = true;
  }
  bool start = true;
  UpdateProcessing(start);
  return 0;
}

int32_t RTAudioCaptureModule::StopRecording() {
  bool start = false;
  {
    rtc::CritScope cs(&crit_);
    recording_ = false;
    start = ShouldStartProcessing();
  }
  UpdateProcessing(start);
  return 0;
}

bool RTAudioCaptureModule::Recording() const {
  rtc::CritScope cs(&crit_);
  return recording_;
}

int32_t RTAudioCaptureModule::InitSpeaker() {
  // No speaker, just playing from file. Return success.
  return 0;
}

bool RTAudioCaptureModule::SpeakerIsInitialized() const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::InitMicrophone() {
  // No microphone, just playing from file. Return success.
  return 0;
}

bool RTAudioCaptureModule::MicrophoneIsInitialized() const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::SpeakerVolumeIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::SetSpeakerVolume(uint32_t /*volume*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::SpeakerVolume(uint32_t* /*volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::MaxSpeakerVolume(
  uint32_t* /*max_volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::MinSpeakerVolume(
  uint32_t* /*min_volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::MicrophoneVolumeIsAvailable(
  bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::SetMicrophoneVolume(uint32_t volume) {
  rtc::CritScope cs(&crit_);
  current_mic_level_ = volume;
  return 0;
}

int32_t RTAudioCaptureModule::MicrophoneVolume(uint32_t* volume) const {
  rtc::CritScope cs(&crit_);
  *volume = current_mic_level_;
  return 0;
}

int32_t RTAudioCaptureModule::MaxMicrophoneVolume(
  uint32_t* max_volume) const {
  *max_volume = kMaxVolume;
  return 0;
}

int32_t RTAudioCaptureModule::MinMicrophoneVolume(
  uint32_t* /*min_volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::SpeakerMuteIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::SetSpeakerMute(bool /*enable*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::SpeakerMute(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::MicrophoneMuteIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::SetMicrophoneMute(bool /*enable*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::MicrophoneMute(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::StereoPlayoutIsAvailable(
  bool* available) const {
  // No recording device, just dropping audio. Stereo can be dropped just
  // as easily as mono.
  *available = true;
  return 0;
}

int32_t RTAudioCaptureModule::SetStereoPlayout(bool /*enable*/) {
  // No recording device, just dropping audio. Stereo can be dropped just
  // as easily as mono.
  return 0;
}

int32_t RTAudioCaptureModule::StereoPlayout(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::StereoRecordingIsAvailable(
  bool* available) const {
  // Keep thing simple. No stereo recording.
  *available = false;
  return 0;
}

int32_t RTAudioCaptureModule::SetStereoRecording(bool enable) {
  if (!enable) {
    return 0;
  }
  return -1;
}

int32_t RTAudioCaptureModule::StereoRecording(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioCaptureModule::PlayoutDelay(uint16_t* delay_ms) const {
  // No delay since audio frames are dropped.
  *delay_ms = 0;
  return 0;
}

void RTAudioCaptureModule::OnMessage(rtc::Message* msg) {
  switch (msg->message_id) {
    case MSG_START_PROCESS:
      StartProcessP();
      break;
    case MSG_RUN_PROCESS:
      ProcessFrameP();
      break;
    default:
      // All existing messages should be caught. Getting here should never
      // happen.
      RTC_NOTREACHED();
  }
}

bool RTAudioCaptureModule::Initialize() {
  // Set the send buffer samples high enough that it would not occur on the
  // remote side unless a packet containing a sample of that magnitude has been
  // sent to it. Note that the audio processing pipeline will likely distort the
  // original signal.
  SetSendBuffer(kHighSampleValue);
  return true;
}

void RTAudioCaptureModule::SetSendBuffer(int value) {
  Sample* buffer_ptr = reinterpret_cast<Sample*>(send_buffer_);
  const size_t buffer_size_in_samples =
    sizeof(send_buffer_) / kNumberBytesPerSample;
  for (size_t i = 0; i < buffer_size_in_samples; ++i) {
    buffer_ptr[i] = value;
  }
}

void RTAudioCaptureModule::ResetRecBuffer() {
  memset(rec_buffer_, 0, sizeof(rec_buffer_));
}

bool RTAudioCaptureModule::CheckRecBuffer(int value) {
  const Sample* buffer_ptr = reinterpret_cast<const Sample*>(rec_buffer_);
  const size_t buffer_size_in_samples =
    sizeof(rec_buffer_) / kNumberBytesPerSample;
  for (size_t i = 0; i < buffer_size_in_samples; ++i) {
    if (buffer_ptr[i] >= value)
      return true;
  }
  return false;
}

bool RTAudioCaptureModule::ShouldStartProcessing() {
  return recording_ || playing_;
}

void RTAudioCaptureModule::UpdateProcessing(bool start) {
  if (start) {
    if (!process_thread_) {
      process_thread_ = rtc::Thread::Create();
      process_thread_->Start();
    }
    process_thread_->Post(RTC_FROM_HERE, this, MSG_START_PROCESS);
  } else {
    if (process_thread_) {
      process_thread_->Stop();
      process_thread_.reset(nullptr);
    }
    started_ = false;
  }
}

void RTAudioCaptureModule::StartProcessP() {
  RTC_CHECK(process_thread_->IsCurrent());
  if (started_) {
    // Already started.
    return;
  }
  ProcessFrameP();
}

void RTAudioCaptureModule::ProcessFrameP() {
  RTC_CHECK(process_thread_->IsCurrent());
  if (!started_) {
    next_frame_time_ = rtc::TimeMillis();
    started_ = true;
  }

  {
    rtc::CritScope cs(&crit_);
    // Receive and send frames every kTimePerFrameMs.
    if (playing_) {
      ReceiveFrameP();
    }
    if (recording_) {
      SendFrameP();
    }
  }

  next_frame_time_ += kTimePerFrameMs;
  const int64_t current_time = rtc::TimeMillis();
  const int64_t wait_time =
    (next_frame_time_ > current_time) ? next_frame_time_ - current_time : 0;
  process_thread_->PostDelayed(RTC_FROM_HERE, wait_time, this, MSG_RUN_PROCESS);
}

void RTAudioCaptureModule::ReceiveFrameP() {
  RTC_CHECK(process_thread_->IsCurrent());
  {
    rtc::CritScope cs(&crit_callback_);
    if (!audio_callback_) {
      return;
    }
    ResetRecBuffer();
    size_t nSamplesOut = 0;
    int64_t elapsed_time_ms = 0;
    int64_t ntp_time_ms = 0;
    if (audio_callback_->NeedMorePlayData(
      kNumberSamples, kNumberBytesPerSample, kNumberOfChannels,
      kSamplesPerSecond, rec_buffer_, nSamplesOut, &elapsed_time_ms,
      &ntp_time_ms) != 0) {
      RTC_NOTREACHED();
    }
    RTC_CHECK(nSamplesOut == kNumberSamples);
  }
  // The SetBuffer() function ensures that after decoding, the audio buffer
  // should contain samples of similar magnitude (there is likely to be some
  // distortion due to the audio pipeline). If one sample is detected to
  // have the same or greater magnitude somewhere in the frame, an actual frame
  // has been received from the remote side (i.e. faked frames are not being
  // pulled).
  if (CheckRecBuffer(kHighSampleValue)) {
    rtc::CritScope cs(&crit_);
    ++frames_received_;
  }
}

void RTAudioCaptureModule::SendFrameP() {
  RTC_CHECK(process_thread_->IsCurrent());
  rtc::CritScope cs(&crit_callback_);
  if (!audio_callback_) {
    return;
  }
  bool key_pressed = false;
  uint32_t current_mic_level = 0;
  MicrophoneVolume(&current_mic_level);
  if (audio_callback_->RecordedDataIsAvailable(
    send_buffer_, kNumberSamples, kNumberBytesPerSample,
    kNumberOfChannels, kSamplesPerSecond, kTotalDelayMs, kClockDriftMs,
    current_mic_level, key_pressed, current_mic_level) != 0) {
    RTC_NOTREACHED();
  }
  SetMicrophoneVolume(current_mic_level);
}