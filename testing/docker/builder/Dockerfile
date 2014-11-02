FROM          quay.io/mozilla/base:0.0.2
MAINTAINER    Jonas Finnemann Jensen <jopsen@gmail.com>

ENV PATH $PATH:/home/worker/bin/
ENV TOOLTOOL_CACHE  /home/worker/tools/tooltool_cache

# Add utilities and configuration
RUN           mkdir                 /home/worker/bin
RUN           mkdir                 /home/worker/tools
ADD           bin                   /home/worker/bin
RUN           chown -R              worker:worker /home/worker/

USER worker

# Set a default command useful for debugging
CMD ["/bin/bash", "--login"]
