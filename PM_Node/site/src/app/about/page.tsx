import type { Metadata } from "next";
import { Section } from "@/components/section";
import { Reveal } from "@/components/reveal";

export const metadata: Metadata = {
  title: "About",
  description: "Why PM Node exists, and two of the engineering decisions along the way.",
};

const NOTES = [
  {
    title: "Separating the TFT and vibration sensor onto their own SPI buses",
    body: "The display would corrupt whenever the vibration sensor initialized, even though each worked fine alone. The vibration sensor was sharing the TFT's SPI peripheral — moving it to its own bus turned a confusing display bug into a clear lesson about peripheral ownership on an ESP32-class board.",
  },
  {
    title: "Replacing the joystick with a rotary encoder",
    body: "The original joystick-and-pointer UI kept fighting calibration drift and unstable readings. A KY-040 rotary encoder — rotate to page, press to record — removed the pointer entirely and matched how the device is actually used: one hand, on the floor, glancing at a page.",
  },
];

export default function AboutPage() {
  return (
    <Section eyebrow="About" title="Why PM Node exists" kicker="One node, not five tools">
      <div className="max-w-[64ch] space-y-5 text-ink-soft mb-14">
        <p>
          PM Node is a portable predictive-maintenance edge device built to
          inspect equipment using multiple sensor modalities on a single
          handheld node — vibration, thermal imaging, probe temperature and
          sound, on one ESP32-S3 board with an on-device TFT, rotary
          navigation, microSD recording and a local web UI.
        </p>
        <p>
          It exists because inspection work usually means carrying several
          disconnected instruments and hoping the readings from each line
          up. PM Node turns that into one compact field tool instead — the
          project is documented in detail, warts and all, on the{" "}
          <a
            href="https://github.com/cnabolouri/cbm/tree/main/PM_Node"
            className="text-accent hover:text-accent-700 underline underline-offset-4"
          >
            GitHub repo
          </a>
          .
        </p>
      </div>

      <span className="block font-mono text-[11px] tracking-[0.12em] uppercase text-accent-700 mb-5">
        Two decisions along the way
      </span>
      <div className="grid sm:grid-cols-2 gap-6">
        {NOTES.map((n) => (
          <Reveal key={n.title}>
            <div className="border border-divider p-6 h-full grid gap-3 content-start">
              <span className="font-heading text-[19px] leading-snug">{n.title}</span>
              <p className="text-[13px] text-muted">{n.body}</p>
            </div>
          </Reveal>
        ))}
      </div>
    </Section>
  );
}
