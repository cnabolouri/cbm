# PM Node — project site

The public site for PM Node, at [pm-node.sinabolouri.com](https://pm-node.sinabolouri.com).
Next.js (App Router) + TypeScript + Tailwind v4, deployed on Vercel.

The firmware this site describes lives one level up, in [`../`](../).

## Local development

```bash
npm install
npm run dev      # http://localhost:3000
npm run build    # production build
npm run lint
```

## Environment

Copy `.env.example` to `.env.local` and fill in the three Resend values to make
the contact form actually send. Without them the form still renders and
validates input, but submissions return a "not configured yet" message rather
than failing silently or crashing.

| Variable | Purpose |
| --- | --- |
| `RESEND_API_KEY` | Resend API key |
| `CONTACT_EMAIL_FROM` | Verified sender, e.g. `PM Node <hello@pm-node.sinabolouri.com>` |
| `CONTACT_EMAIL_TO` | Where submissions are delivered |

The sending domain must be verified in Resend before mail will go out.

## Structure

```
src/app/            one folder per route (/, solution, hardware, use, status, about, contact)
src/app/contact/    page + client form + the "use server" action that sends via Resend
src/components/     site-header, site-footer, animated-mark, reveal, section
src/components/ui/  button, tag, grid primitives over the design tokens
src/app/globals.css design tokens (@theme) + the logo's pulse/breathe keyframes
public/             brand SVGs from the Claude Design handoff bundle
```

Design tokens (colors, type scale, the blueprint/hairline treatment) come from
the "Industry" design system in the handoff bundle at
`../wireframe-surfaces-and-icon-direction/`.

## Deploying to Vercel

The repo root is `cbm`, so the Vercel project must be told this app lives in a
subdirectory.

1. Vercel → **Add New… → Project** → import `cnabolouri/cbm`.
2. Set **Root Directory** to `PM_Node/site`. Framework preset: Next.js (auto-detected).
3. Add `RESEND_API_KEY`, `CONTACT_EMAIL_FROM`, `CONTACT_EMAIL_TO` under
   Settings → Environment Variables (Production + Preview).
4. Settings → **Domains** → add `pm-node.sinabolouri.com`.

### Before step 4: the domain is currently pointed at GitHub Pages

`pm-node.sinabolouri.com` currently resolves to `cnabolouri.github.io`
(GitHub Pages, serving a 404), because of the `CNAME` file in the root of this
repo. Vercel cannot serve the domain until that is released:

- Point the DNS record for `pm-node` at Vercel (`cname.vercel-dns.com`) instead
  of `cnabolouri.github.io`, at whatever DNS provider hosts `sinabolouri.com`.
- Disable GitHub Pages for this repo (Settings → Pages) and delete the root
  `CNAME` file — otherwise Pages keeps re-claiming the domain.
