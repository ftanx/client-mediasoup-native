#include <string.h>
#include <iostream>

#include "RtAudioDeviceModule.hpp"
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

RTAudioDeviceModule::RTAudioDeviceModule()
  : audio_callback_(nullptr),
    recording_(false),
    playing_(false),
    play_is_initialized_(false),
    rec_is_initialized_(false),
    current_mic_level_(kMaxVolume),
    started_(false),
    next_frame_time_(0),
    frames_received_(0) {}

RTAudioDeviceModule::~RTAudioDeviceModule() {
  if (process_thread_) {
    process_thread_->Stop();
  }
}

rtc::scoped_refptr<RTAudioDeviceModule> RTAudioDeviceModule::Create() {
  rtc::scoped_refptr<RTAudioDeviceModule> capture_module(
    new rtc::RefCountedObject<RTAudioDeviceModule>());
  if (!capture_module->Initialize()) {
    return nullptr;
  }
  return capture_module;
}

int RTAudioDeviceModule::frames_received() const {
  rtc::CritScope cs(&crit_);
  return frames_received_;
}

int32_t RTAudioDeviceModule::ActiveAudioLayer(
  AudioLayer* /*audio_layer*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::RegisterAudioCallback(
  webrtc::AudioTransport* audio_callback) {
  rtc::CritScope cs(&crit_callback_);
  audio_callback_ = audio_callback;
  return 0;
}

int32_t RTAudioDeviceModule::Init() {
  // Initialize is called by the factory method. Safe to ignore this Init call.
  std::cout << "yes" << std::endl;
  //SetSendBuffer(4);
  return 0;
}

int32_t RTAudioDeviceModule::Terminate() {
  // Clean up in the destructor. No action here, just success.
  return 0;
}

bool RTAudioDeviceModule::Initialized() const {
  RTC_NOTREACHED();
  return 0;
}

int16_t RTAudioDeviceModule::PlayoutDevices() {
  RTC_NOTREACHED();
  return 0;
}

int16_t RTAudioDeviceModule::RecordingDevices() {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::PlayoutDeviceName(
  uint16_t /*index*/,
  char /*name*/[webrtc::kAdmMaxDeviceNameSize],
  char /*guid*/[webrtc::kAdmMaxGuidSize]) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::RecordingDeviceName(
  uint16_t /*index*/,
  char /*name*/[webrtc::kAdmMaxDeviceNameSize],
  char /*guid*/[webrtc::kAdmMaxGuidSize]) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::SetPlayoutDevice(uint16_t /*index*/) {
  // No playout device, just playing from file. Return success.
  return 0;
}

int32_t RTAudioDeviceModule::SetPlayoutDevice(WindowsDeviceType /*device*/) {
  if (play_is_initialized_) {
    return -1;
  }
  return 0;
}

int32_t RTAudioDeviceModule::SetRecordingDevice(uint16_t /*index*/) {
  // No recording device, just dropping audio. Return success.
  return 0;
}

int32_t RTAudioDeviceModule::SetRecordingDevice(
  WindowsDeviceType /*device*/) {
  if (rec_is_initialized_) {
    return -1;
  }
  return 0;
}

int32_t RTAudioDeviceModule::PlayoutIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::InitPlayout() {
  play_is_initialized_ = true;
  return 0;
}

bool RTAudioDeviceModule::PlayoutIsInitialized() const {
  return play_is_initialized_;
}

int32_t RTAudioDeviceModule::RecordingIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::InitRecording() {
  rec_is_initialized_ = true;
  return 0;
}

bool RTAudioDeviceModule::RecordingIsInitialized() const {
  return rec_is_initialized_;
}

int32_t RTAudioDeviceModule::StartPlayout() {
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

int32_t RTAudioDeviceModule::StopPlayout() {
  bool start = false;
  {
    rtc::CritScope cs(&crit_);
    playing_ = false;
    start = ShouldStartProcessing();
  }
  UpdateProcessing(start);
  return 0;
}

bool RTAudioDeviceModule::Playing() const {
  rtc::CritScope cs(&crit_);
  return playing_;
}

int32_t RTAudioDeviceModule::StartRecording() {
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

int32_t RTAudioDeviceModule::StopRecording() {
  bool start = false;
  {
    rtc::CritScope cs(&crit_);
    recording_ = false;
    start = ShouldStartProcessing();
  }
  UpdateProcessing(start);
  return 0;
}

bool RTAudioDeviceModule::Recording() const {
  rtc::CritScope cs(&crit_);
  return recording_;
}

int32_t RTAudioDeviceModule::InitSpeaker() {
  // No speaker, just playing from file. Return success.
  return 0;
}

bool RTAudioDeviceModule::SpeakerIsInitialized() const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::InitMicrophone() {
  // No microphone, just playing from file. Return success.
  return 0;
}

bool RTAudioDeviceModule::MicrophoneIsInitialized() const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::SpeakerVolumeIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::SetSpeakerVolume(uint32_t /*volume*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::SpeakerVolume(uint32_t* /*volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::MaxSpeakerVolume(
  uint32_t* /*max_volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::MinSpeakerVolume(
  uint32_t* /*min_volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::MicrophoneVolumeIsAvailable(
  bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::SetMicrophoneVolume(uint32_t volume) {
  rtc::CritScope cs(&crit_);
  current_mic_level_ = volume;
  return 0;
}

int32_t RTAudioDeviceModule::MicrophoneVolume(uint32_t* volume) const {
  rtc::CritScope cs(&crit_);
  *volume = current_mic_level_;
  return 0;
}

int32_t RTAudioDeviceModule::MaxMicrophoneVolume(
  uint32_t* max_volume) const {
  *max_volume = kMaxVolume;
  return 0;
}

int32_t RTAudioDeviceModule::MinMicrophoneVolume(
  uint32_t* /*min_volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::SpeakerMuteIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::SetSpeakerMute(bool /*enable*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::SpeakerMute(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::MicrophoneMuteIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::SetMicrophoneMute(bool /*enable*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::MicrophoneMute(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::StereoPlayoutIsAvailable(
  bool* available) const {
  // No recording device, just dropping audio. Stereo can be dropped just
  // as easily as mono.
  *available = true;
  return 0;
}

int32_t RTAudioDeviceModule::SetStereoPlayout(bool /*enable*/) {
  // No recording device, just dropping audio. Stereo can be dropped just
  // as easily as mono.
  return 0;
}

int32_t RTAudioDeviceModule::StereoPlayout(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::StereoRecordingIsAvailable(
  bool* available) const {
  // Keep thing simple. No stereo recording.
  *available = false;
  return 0;
}

int32_t RTAudioDeviceModule::SetStereoRecording(bool enable) {
  if (!enable) {
    return 0;
  }
  return -1;
}

int32_t RTAudioDeviceModule::StereoRecording(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t RTAudioDeviceModule::PlayoutDelay(uint16_t* delay_ms) const {
  // No delay since audio frames are dropped.
  *delay_ms = 0;
  return 0;
}

void RTAudioDeviceModule::OnMessage(rtc::Message* msg) {
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

bool RTAudioDeviceModule::Initialize() {
  // Set the send buffer samples high enough that it would not occur on the
  // remote side unless a packet containing a sample of that magnitude has been
  // sent to it. Note that the audio processing pipeline will likely distort the
  // original signal.
  SetSendBuffer(kHighSampleValue);
  return true;
}

void RTAudioDeviceModule::SetSendBuffer(int value) {
  Sample* buffer_ptr = reinterpret_cast<Sample*>(send_buffer_);
  const size_t buffer_size_in_samples =
    sizeof(send_buffer_) / kNumberBytesPerSample;
  for (size_t i = 0; i < buffer_size_in_samples; ++i) {
    buffer_ptr[i] = value;
  }
}

void RTAudioDeviceModule::ResetRecBuffer() {
  memset(rec_buffer_, 0, sizeof(rec_buffer_));
}

bool RTAudioDeviceModule::CheckRecBuffer(int value) {
  const Sample* buffer_ptr = reinterpret_cast<const Sample*>(rec_buffer_);
  const size_t buffer_size_in_samples =
    sizeof(rec_buffer_) / kNumberBytesPerSample;
  for (size_t i = 0; i < buffer_size_in_samples; ++i) {
    if (buffer_ptr[i] >= value)
      return true;
  }
  return false;
}

bool RTAudioDeviceModule::ShouldStartProcessing() {
  return recording_ || playing_;
}

void RTAudioDeviceModule::UpdateProcessing(bool start) {
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

void RTAudioDeviceModule::StartProcessP() {
  RTC_CHECK(process_thread_->IsCurrent());
  if (started_) {
    // Already started.
    return;
  }
  ProcessFrameP();
}

void RTAudioDeviceModule::ProcessFrameP() {
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

void RTAudioDeviceModule::ReceiveFrameP() {
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

void RTAudioDeviceModule::SendFrameP() {
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