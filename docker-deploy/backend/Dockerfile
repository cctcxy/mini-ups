FROM xiea/miniups_backend:miniups
ADD . /BackendApp
WORKDIR /BackendApp
RUN make protoc
RUN make