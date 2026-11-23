import { SpectrumAnalyzer, computeWaveform, rms, peak, runSelfTest } from '@quizzyjoy/audio-dsp';

const SAMPLE_RATE: number = 48000;
const CHUNK_SIZE: number = 960;   // 20 ms @ 48 kHz
const FFT_SIZE: number = 1024;
const BAND_COUNT: number = 32;
const WAVE_POINTS: number = 96;
const TICK_MS: number = 33;       // ~30 fps UI

@Entry
@Component
struct Index {
  @State bands: number[] = [];
  @State wave: number[] = [];
  @State frequency: number = 440;
  @State running: boolean = true;
  @State rmsText: string = '0.000';
  @State peakText: string = '0.000';
  @State peakBandText: string = '-';
  @State frameCount: number = 0;
  @State selfTestText: string = '—';

  private analyzer: SpectrumAnalyzer = new SpectrumAnalyzer({
    fftSize: FFT_SIZE,
    sampleRate: SAMPLE_RATE,
    bands: BAND_COUNT
  });
  private timerId: number = -1;
  private phase: number = 0;
  private frames: number = 0;
  private lastFrames: number = 0;
  private lastTick: number = 0;

  aboutToAppear(): void {
    const zeros: number[] = [];
    for (let i = 0; i < BAND_COUNT; i++) {
      zeros.push(0);
    }
    this.bands = zeros;

    const flat: number[] = [];
    for (let i = 0; i < WAVE_POINTS; i++) {
      flat.push(0);
    }
    this.wave = flat;

    this.selfTestText = runSelfTest() ? 'PASS' : 'FAIL';
    this.lastTick = Date.now();
    this.start();
  }

  aboutToDisappear(): void {
    this.stop();
    this.analyzer.release();
  }

  private start(): void {
    if (this.timerId !== -1) {
      return;
    }
    this.timerId = setInterval(() => {
      this.tick();
    }, TICK_MS);
    this.running = true;
  }

  private stop(): void {
    if (this.timerId !== -1) {
      clearInterval(this.timerId);
      this.timerId = -1;
    }
    this.running = false;
  }

  /** Synthesize one chunk of a tone (fundamental + 2nd/3rd harmonics) and analyze it. */
  private tick(): void {
    if (!this.analyzer.ready) {
      return;
    }

    const pcm = new Int16Array(CHUNK_SIZE);
    const step = (2 * Math.PI * this.frequency) / SAMPLE_RATE;
    for (let i = 0; i < CHUNK_SIZE; i++) {
      const s = Math.sin(this.phase) * 0.55
        + Math.sin(this.phase * 2) * 0.18
        + Math.sin(this.phase * 3) * 0.08;
      pcm[i] = Math.round(s * 18000);
      this.phase += step;
      if (this.phase > Math.PI * 2) {
        this.phase -= Math.PI * 2;
      }
    }

    const frame = this.analyzer.push(pcm);
    if (frame !== null) {
      const next: number[] = [];
      let peakBand = 0;
      let peakValue = 0;
      for (let i = 0; i < frame.length; i++) {
        const v = frame[i];
        next.push(v);
        if (v > peakValue) {
          peakValue = v;
          peakBand = i;
        }
      }
      this.bands = next;
      this.peakBandText = `${peakBand} / ${frame.length}`;
      this.frames++;
    }

    const envelope = computeWaveform(pcm, WAVE_POINTS);
    const waveNext: number[] = [];
    for (let i = 0; i < WAVE_POINTS; i++) {
      const lo = Math.abs(envelope[i * 2]);
      const hi = Math.abs(envelope[i * 2 + 1]);
      waveNext.push(lo > hi ? lo : hi);
    }
    this.wave = waveNext;

    this.rmsText = rms(pcm).toFixed(3);
    this.peakText = peak(pcm).toFixed(3);

    const now = Date.now();
    if (now - this.lastTick >= 1000) {
      this.frameCount = this.frames - this.lastFrames;
      this.lastFrames = this.frames;
      this.lastTick = now;
    }
  }

  private barColor(index: number): ResourceColor {
    const t = index / Math.max(1, BAND_COUNT - 1);
    if (t < 0.5) {
      return '#3B82F6';
    }
    if (t < 0.8) {
      return '#22D3EE';
    }
    return '#F472B6';
  }

  build() {
    Column() {
      Row() {
        Text('audio-dsp')
          .fontSize(22)
          .fontWeight(FontWeight.Bold)
          .fontColor('#E5E7EB')
        Blank()
        Text(`selftest ${this.selfTestText}  ·  ${this.running ? 'RUNNING' : 'PAUSED'}`)
          .fontSize(12)
          .fontColor(this.selfTestText === 'PASS' ? '#34D399' : '#F87171')
      }
      .width('100%')

      Text('FFT spectrum · waveform · level metering — synthetic tone, no microphone permission')
        .fontSize(12)
        .fontColor('#94A3B8')
        .margin({ top: 4, bottom: 16 })
        .width('100%')

      // ---- spectrum ----
      Row({ space: 3 }) {
        ForEach(this.bands, (value: number, index: number) => {
          Column()
            .width(9)
            .height(8 + value * 150)
            .borderRadius(3)
            .backgroundColor(this.barColor(index))
        }, (value: number, index: number) => index.toString())
      }
      .width('100%')
      .height(170)
      .alignItems(VerticalAlign.Bottom)
      .padding({ left: 4, right: 4 })

      // ---- waveform ----
      Row({ space: 1 }) {
        ForEach(this.wave, (value: number, index: number) => {
          Column()
            .width(3)
            .height(2 + value * 70)
            .backgroundColor('#64748B')
        }, (value: number, index: number) => index.toString())
      }
      .width('100%')
      .height(80)
      .alignItems(VerticalAlign.Center)
      .margin({ top: 10 })

      // ---- readouts ----
      Row({ space: 24 }) {
        this.readout('freq', `${this.frequency.toFixed(0)} Hz`)
        this.readout('rms', this.rmsText)
        this.readout('peak', this.peakText)
        this.readout('peak band', this.peakBandText)
        this.readout('frames/s', this.frameCount.toString())
      }
      .width('100%')
      .margin({ top: 14, bottom: 8 })

      // ---- controls ----
      Row({ space: 10 }) {
        Text('频率')
          .fontSize(13)
          .fontColor('#CBD5E1')
          .width(36)
        Slider({
          value: this.frequency,
          min: 110,
          max: 2000,
          step: 10,
          style: SliderStyle.OutSet
        })
          .layoutWeight(1)
          .onChange((value: number) => {
            this.frequency = value;
          })
        Text(`${this.frequency.toFixed(0)} Hz`)
          .fontSize(13)
          .fontColor('#E2E8F0')
          .width(70)
          .textAlign(TextAlign.End)
      }
      .width('100%')
      .margin({ top: 6 })

      Row({ space: 12 }) {
        Button(this.running ? '暂停' : '继续')
          .onClick(() => {
            if (this.running) {
              this.stop();
            } else {
              this.start();
            }
          })
        Button('重置分析器')
          .backgroundColor('#334155')
          .onClick(() => {
            this.analyzer.reset();
          })
      }
      .width('100%')
      .margin({ top: 12 })

      Blank()
    }
    .width('100%')
    .height('100%')
    .padding(20)
    .backgroundColor('#0B1020')
  }

  @Builder
  readout(label: string, value: string) {
    Column() {
      Text(label)
        .fontSize(10)
        .fontColor('#64748B')
      Text(value)
        .fontSize(14)
        .fontColor('#E2E8F0')
        .margin({ top: 2 })
    }
    .alignItems(HorizontalAlign.Start)
  }
}
