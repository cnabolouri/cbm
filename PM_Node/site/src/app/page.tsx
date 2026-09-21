import Link from "next/link";
import { AlertTriangle, Cpu, CircleDot, GitCommitHorizontal, ArrowRight } from "lucide-react";
import { AnimatedMark } from "@/components/animated-mark";
import { Reveal } from "@/components/reveal";
import { Button } from "@/components/ui/button";
import { Tag } from "@/components/ui/tag";

const HIGHLIGHTS = [
  {
    href: "/solution",
    icon: AlertTriangle,
    title: "The problem",
    body: "Five tools, five walks to the machine — vibration, thermal, temperature and sound checks that never line up.",
  },
  {
    href: "/hardware",
    icon: Cpu,
    title: "Six sensors, one board",
    body: "IIS3DWB, MLX90640, DS18B20 and an I2S mic feeding one ESP32-S3, with a TFT, a rotary encoder and a web UI.",
  },
  {
    href: "/use",
    icon: CircleDot,
    title: "One control",
    body: "The joystick came out; a rotary went in. Rotate to page through readings, press to start or stop recording.",
  },
  {
    href: "/status",
    icon: GitCommitHorizontal,
    title: "Where it stands",
    body: "Five hardware revisions in, actively developed, and honest about what's still not production ready.",
  },
];

export default function Home() {
  return (
    <>
      <section className="max-w-5xl mx-auto px-5 pt-16 pb-20 sm:pt-24 sm:pb-28 grid gap-12 lg:grid-cols-[1.2fr_1fr] items-center">
        <Reveal>
          <span className="font-mono text-[11px] tracking-[0.12em] uppercase text-accent-text">
            PM Node
          </span>
          <h1 className="mt-3 text-[38px] sm:text-[52px] max-w-[18ch]">
            Four inspection instruments in one handheld node.
          </h1>
          <p className="mt-5 max-w-[48ch] text-ink-soft">
            Vibration, thermal, probe temperature and sound on one ESP32-S3
            device, with an on-board TFT, a rotary interface, microSD
            recording and a local web UI over its own access point.
          </p>
          <div className="mt-7 flex flex-wrap gap-3">
            <Button href="/solution">Explore the solution</Button>
            <Button href="https://github.com/cnabolouri/cbm/tree/main/PM_Node" variant="secondary">
              Read the source
            </Button>
          </div>
          <div className="mt-5 flex flex-wrap gap-2">
            <Tag>In progress</Tag>
            <Tag>ESP32-S3</Tag>
            <Tag>Arduino / C++</Tag>
          </div>
        </Reveal>

        <Reveal delay={0.1}>
          <div className="aspect-square border border-divider flex items-center justify-center">
            <AnimatedMark size={180} />
          </div>
        </Reveal>
      </section>

      <section className="border-t border-divider">
        <div className="max-w-5xl mx-auto px-5 py-16">
          <Reveal>
            <div className="grid sm:grid-cols-2 gap-px bg-divider border border-divider">
              {HIGHLIGHTS.map(({ href, icon: Icon, title, body }) => (
                <Link
                  key={href}
                  href={href}
                  className="group bg-bg p-6 flex flex-col gap-3 hover:bg-accent-tint transition-colors"
                >
                  <Icon size={22} strokeWidth={1.5} className="text-accent" />
                  <span className="font-heading text-[20px]">{title}</span>
                  <p className="text-[13px] text-muted flex-1">{body}</p>
                  <span className="inline-flex items-center gap-1 font-mono text-[10px] uppercase tracking-[0.08em] text-accent-text group-hover:gap-2 transition-all">
                    More <ArrowRight size={12} />
                  </span>
                </Link>
              ))}
            </div>
          </Reveal>
        </div>
      </section>

      <section className="bg-feature text-feature-fg">
        <div className="max-w-5xl mx-auto px-5 py-16 flex flex-wrap items-center justify-between gap-6">
          <Reveal>
            <h2 className="text-[26px] max-w-[24ch]">
              Built to replace five walks to the machine with one.
            </h2>
          </Reveal>
          <Reveal delay={0.1}>
            <Button href="/contact">Get in touch</Button>
          </Reveal>
        </div>
      </section>
    </>
  );
}
