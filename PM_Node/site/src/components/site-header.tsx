"use client";

import Link from "next/link";
import Image from "next/image";
import { usePathname } from "next/navigation";
import { useState } from "react";
import { Menu, X } from "lucide-react";
import { AnimatePresence, motion } from "framer-motion";

const LINKS = [
  { href: "/solution", label: "Solution" },
  { href: "/hardware", label: "Hardware" },
  { href: "/use", label: "Use" },
  { href: "/status", label: "Status" },
  { href: "/about", label: "About" },
];

export function SiteHeader() {
  const pathname = usePathname();
  const [open, setOpen] = useState(false);

  return (
    <header className="sticky top-0 z-50 bg-bg/90 backdrop-blur border-b border-divider">
      <div className="max-w-5xl mx-auto px-5 h-16 flex items-center justify-between gap-4">
        <Link href="/" className="flex items-center gap-2" onClick={() => setOpen(false)}>
          <Image src="/logo-full-color.svg" alt="PM Node" width={140} height={41} className="h-7 w-auto" priority />
        </Link>

        <nav className="hidden md:flex items-center gap-7 font-mono text-[11px] tracking-[0.08em] uppercase text-muted">
          {LINKS.map((l) => (
            <Link
              key={l.href}
              href={l.href}
              className={pathname === l.href ? "text-accent" : "hover:text-ink"}
            >
              {l.label}
            </Link>
          ))}
          <Link
            href="/contact"
            className="bg-accent text-bg px-3.5 py-2 -my-2 hover:bg-accent-600"
          >
            Contact
          </Link>
        </nav>

        <button
          type="button"
          aria-label={open ? "Close menu" : "Open menu"}
          className="md:hidden text-ink"
          onClick={() => setOpen((v) => !v)}
        >
          {open ? <X size={22} /> : <Menu size={22} />}
        </button>
      </div>

      <AnimatePresence>
        {open && (
          <motion.nav
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: "auto", opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            transition={{ duration: 0.2 }}
            className="md:hidden overflow-hidden border-t border-divider bg-bg"
          >
            <div className="flex flex-col px-5 py-4 gap-4 font-mono text-[12px] tracking-[0.06em] uppercase text-muted">
              {LINKS.map((l) => (
                <Link
                  key={l.href}
                  href={l.href}
                  onClick={() => setOpen(false)}
                  className={pathname === l.href ? "text-accent" : ""}
                >
                  {l.label}
                </Link>
              ))}
              <Link href="/contact" onClick={() => setOpen(false)} className="text-accent">
                Contact
              </Link>
            </div>
          </motion.nav>
        )}
      </AnimatePresence>
    </header>
  );
}
