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
  @State bass: number = 0;
  @State mid: number = 0;
  @State treble: number = 0;
  @State overlap: boolean = false;

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

    const frame = this.analyzer.pushFrame(pcm);
    if (frame !== null) {
      const next: number[] = [];
      let peakBand = 0;
      let peakValue = 0;
      const values = frame.bands;
      for (let i = 0; i < values.length; i++) {
        const v = values[i];
        next.push(v);
        if (v > peakValue) {
          peakValue = v;
          peakBand = i;
        }
      }
      this.bands = next;
      this.peakBandText = `${peakBand} / ${values.length}`;
      this.bass = frame.bass;
      this.mid = frame.mid;
      this.treble = frame.treble;
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

  private rebuildAnalyzer(): void {
    this.analyzer.release();
    this.analyzer = new SpectrumAnalyzer({
      fftSize: FFT_SIZE,
      sampleRate: SAMPLE_RATE,
      bands: BAND_COUNT,
      overlap: this.overlap ? 50 : 0
    });
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
        }, (value: number, index: number) => `${index}-${value.toFixed(4)}`)
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
        }, (value: number, index: number) => `${index}-${value.toFixed(4)}`)
      }
      .width('100%')
      .height(80)
      .alignItems(VerticalAlign.Center)
      .margin({ top: 10 })

      // ---- readouts ----
      Row({ space: 24 }) {
        Readout({ label: 'freq', value: `${this.frequency.toFixed(0)} Hz` })
        Readout({ label: 'rms', value: this.rmsText })
        Readout({ label: 'peak', value: this.peakText })
        Readout({ label: 'peak band', value: this.peakBandText })
        Readout({ label: 'frames/s', value: this.frameCount.toString() })
      }
      .width('100%')
      .margin({ top: 14, bottom: 8 })

      Row({ space: 12 }) {
        Meter({ label: 'BASS', value: this.bass, color: '#F472B6' })
        Meter({ label: 'MID', value: this.mid, color: '#60A5FA' })
        Meter({ label: 'TREBLE', value: this.treble, color: '#22D3EE' })
      }
      .width('100%')
      .margin({ bottom: 8 })

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

      Row({ space: 10 }) {
        Text('50% 重叠')
          .fontSize(13)
          .fontColor('#CBD5E1')
        Toggle({ type: ToggleType.Switch, isOn: this.overlap })
          .onChange((isOn: boolean) => {
            this.overlap = isOn;
            this.rebuildAnalyzer();
          })
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

}

/**
 * One metric readout. A @Component with @Prop (not a @Builder with value
 * parameters) so the text re-renders when the value changes.
 */
@Component
struct Readout {
  @Prop label: string = '';
  @Prop value: string = '';

  build() {
    Column() {
      Text(this.label)
        .fontSize(10)
        .fontColor('#64748B')
      Text(this.value)
        .fontSize(14)
        .fontColor('#E2E8F0')
        .margin({ top: 2 })
    }
    .alignItems(HorizontalAlign.Start)
  }
}

/**
 * Bass / mid / treble level meter (0..1) with a thin bar.
 */
@Component
struct Meter {
  @Prop label: string = '';
  @Prop value: number = 0;
  @Prop color: string = '#60A5FA';

  build() {
    Column() {
      Row() {
        Text(this.label)
          .fontSize(10)
          .fontColor('#64748B')
        Blank()
        Text(this.value.toFixed(2))
          .fontSize(11)
          .fontColor('#E2E8F0')
      }
      .width('100%')

      Row() {
        Column()
          .width(`${Math.round(this.value * 100)}%`)
          .height(6)
          .borderRadius(3)
          .backgroundColor(this.color)
      }
      .width('100%')
      .height(6)
      .borderRadius(3)
      .backgroundColor('#1E293B')
      .margin({ top: 4 })
    }
    .layoutWeight(1)
  }
}
