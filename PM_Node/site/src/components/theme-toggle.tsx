"use client";

import { useTheme } from "next-themes";
import { Sun, Moon } from "lucide-react";

export function ThemeToggle({ className = "" }: { className?: string }) {
  const { resolvedTheme, setTheme } = useTheme();

  return (
    <button
      type="button"
      onClick={() => setTheme(resolvedTheme === "dark" ? "light" : "dark")}
      aria-label="Toggle light or dark theme"
      className={`inline-flex items-center justify-center w-9 h-9 border border-divider text-muted hover:text-ink hover:border-accent transition-colors ${className}`}
    >
      {/* Which icon shows is decided in CSS off the .dark class, so the server
          renders both and hydration never has to guess the visitor's theme. */}
      <Moon size={16} strokeWidth={1.5} className="dark:hidden" />
      <Sun size={16} strokeWidth={1.5} className="hidden dark:block" />
    </button>
  );
}
