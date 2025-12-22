#pragma once

namespace Video2Card::UI
{

  class UIComponent
  {
public:

    virtual ~UIComponent() = default;

    virtual void Render() = 0;
    virtual void Update() {}
  };

} // namespace Video2Card::UI
