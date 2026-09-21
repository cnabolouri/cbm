import Link from "next/link";
import type { ComponentPropsWithoutRef, ReactNode } from "react";

type Variant = "primary" | "secondary";

const base =
  "inline-flex items-center justify-center gap-2 font-heading text-[17px] font-semibold tracking-[0.03em] px-6 py-3 border transition-colors";

const variants: Record<Variant, string> = {
  primary:
    "bg-accent text-bg border-accent hover:bg-accent-600 hover:border-accent-600",
  secondary:
    "bg-transparent text-ink-soft border-divider hover:bg-ink/5",
};

type BaseProps = {
  variant?: Variant;
  children: ReactNode;
  className?: string;
};

type ButtonAsLink = BaseProps & { href: string } & Omit<
    ComponentPropsWithoutRef<typeof Link>,
    "href" | "className" | "children"
  >;

type ButtonAsButton = BaseProps &
  Omit<ComponentPropsWithoutRef<"button">, "className" | "children"> & {
    href?: undefined;
  };

export function Button(props: ButtonAsLink | ButtonAsButton) {
  const { variant = "primary", children, className = "", ...rest } = props;
  const cls = `${base} ${variants[variant]} ${className}`;

  if ("href" in rest && rest.href) {
    const { href, ...linkRest } = rest as ButtonAsLink;
    return (
      <Link href={href} className={cls} {...linkRest}>
        {children}
      </Link>
    );
  }

  const buttonRest = rest as Omit<ButtonAsButton, "href" | "variant" | "children" | "className">;
  return (
    <button className={cls} {...buttonRest}>
      {children}
    </button>
  );
}
