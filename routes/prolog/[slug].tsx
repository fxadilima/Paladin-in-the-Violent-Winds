import { Handlers, PageProps } from "$fresh/server.ts";
import MarkdownIt from 'markdown-it';
import MarkdownItFootnote from 'markdown-it-footnote';

//import renderToString from 'preact-render-to-string';

// routes/blog/[slug].tsx

interface BlogPostData {
  title: string;
  content: string;
}

interface BlogPostPageProps extends PageProps {
  data: BlogPostData;
}

export const handler: Handlers<BlogPostData> = {
  async GET(req, ctx) {
    const { slug } = ctx.params; // Access the 'slug' parameter here
    const txt = await Deno.readTextFile(`${Deno.cwd()}/yttlj-continue/contents/Prolog/${slug}`);
    const md = new MarkdownIt({html: true}).use(MarkdownItFootnote);
    const html = md.render(txt);
    const page: BlogPostData = {title: "Testing", content: html};
    return ctx.render(page);
  },
};

export default function BlogPost({ data }: BlogPostPageProps) {
  return <div dangerouslySetInnerHTML={{ __html: data.content} } />;
}

