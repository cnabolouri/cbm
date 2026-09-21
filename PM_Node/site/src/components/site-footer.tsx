import Image from "next/image";
import Link from "next/link";

export function SiteFooter() {
  return (
    <footer className="border-t border-divider">
      <div className="max-w-5xl mx-auto px-5 py-8 flex flex-wrap items-center justify-between gap-4 font-mono text-[11px] text-muted">
        <div className="flex items-center gap-3">
          <Image src="/logo-icon-color.svg" alt="PM Node" width={22} height={22} />
          <span>PM Node · an inspection instrument, not (yet) a product</span>
        </div>
        <div className="flex items-center gap-5">
          <Link href="https://github.com/cnabolouri/cbm/tree/main/PM_Node" className="hover:text-accent">
            GitHub ↗
          </Link>
          <Link href="/contact" className="hover:text-accent">
            Contact
          </Link>
        </div>
      </div>
    </footer>
  );
}
