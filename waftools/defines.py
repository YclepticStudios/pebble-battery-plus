def options(ctx):
    ctx.add_option('--build-debug', action='store_true', default=False,
                   help="Mark a build to include debug code")

def configure(ctx):
    if ctx.options.build_debug:
        ctx.env.append_value('DEFINES', 'BUILD_DEBUG')
